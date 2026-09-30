#include "standalone.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <conio.h>
#include <tlhelp32.h>
#endif

namespace Standalone {
    namespace {
        constexpr std::array<char, 8> standalone_magic = {'C', 'O', 'V', 'E', 'B', 'C', '0', '1'};
        constexpr std::size_t standalone_trailer_size = sizeof(std::uint64_t) + standalone_magic.size();

        using ByteBuffer = std::vector<std::uint8_t>;

        void append_uint32(ByteBuffer& buffer, std::uint32_t value) {
            for (unsigned int shift = 0; shift < 32; shift += 8) {
                buffer.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
            }
        }

        void append_uint64(ByteBuffer& buffer, std::uint64_t value) {
            for (unsigned int shift = 0; shift < 64; shift += 8) {
                buffer.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
            }
        }

        std::uint32_t read_uint32(const ByteBuffer& buffer, std::size_t& offset) {
            if (offset > buffer.size() || buffer.size() - offset < sizeof(std::uint32_t)) {
                throw std::runtime_error("Invalid standalone bytecode payload");
            }

            std::uint32_t value = 0;
            for (unsigned int shift = 0; shift < 32; shift += 8) {
                value |= static_cast<std::uint32_t>(buffer[offset++]) << shift;
            }
            return value;
        }

        std::uint64_t read_uint64(const ByteBuffer& buffer, std::size_t& offset) {
            if (offset > buffer.size() || buffer.size() - offset < sizeof(std::uint64_t)) {
                throw std::runtime_error("Invalid standalone bytecode payload");
            }

            std::uint64_t value = 0;
            for (unsigned int shift = 0; shift < 64; shift += 8) {
                value |= static_cast<std::uint64_t>(buffer[offset++]) << shift;
            }
            return value;
        }

        void append_string(ByteBuffer& buffer, const std::string& value) {
            append_uint64(buffer, static_cast<std::uint64_t>(value.size()));
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(value.data());
            buffer.insert(buffer.end(), bytes, bytes + value.size());
        }

        std::string read_string(const ByteBuffer& buffer, std::size_t& offset) {
            const std::uint64_t length = read_uint64(buffer, offset);
            if (length > std::numeric_limits<std::size_t>::max() ||
                offset > buffer.size() || length > buffer.size() - offset) {
                throw std::runtime_error("Invalid standalone bytecode payload");
            }

            const auto string_length = static_cast<std::size_t>(length);
            std::string value(reinterpret_cast<const char*>(buffer.data() + offset), string_length);
            offset += string_length;
            return value;
        }

        ByteBuffer serialize_bytecode(const Compiler::Bytecode& bytecode) {
            ByteBuffer buffer;
            append_uint64(buffer, static_cast<std::uint64_t>(bytecode.size()));

            for (const Instruction& instruction : bytecode) {
                append_uint32(buffer, static_cast<std::uint32_t>(instruction.op));
                if (instruction.operand.is_number()) {
                    buffer.push_back(0);
                    const double number = instruction.operand.as_number();
                    std::uint64_t number_bits = 0;
                    static_assert(sizeof(number) == sizeof(number_bits));
                    std::memcpy(&number_bits, &number, sizeof(number_bits));
                    append_uint64(buffer, number_bits);
                } else if (instruction.operand.is_string()) {
                    buffer.push_back(1);
                    append_string(buffer, instruction.operand.as_string());
                } else {
                    buffer.push_back(2);
                    append_string(buffer, instruction.operand.as_function());
                }
            }

            return buffer;
        }

        Compiler::Bytecode deserialize_bytecode(const ByteBuffer& buffer) {
            std::size_t offset = 0;
            const std::uint64_t instruction_count = read_uint64(buffer, offset);
            if (instruction_count > std::numeric_limits<std::size_t>::max() ||
                instruction_count > (buffer.size() - offset) / 5) {
                throw std::runtime_error("Invalid standalone bytecode payload");
            }

            Compiler::Bytecode bytecode;
            bytecode.reserve(static_cast<std::size_t>(instruction_count));
            for (std::uint64_t index = 0; index < instruction_count; ++index) {
                const std::uint32_t opcode = read_uint32(buffer, offset);
                if (opcode > static_cast<std::uint32_t>(CSTORE)) {
                    throw std::runtime_error("Invalid standalone bytecode opcode");
                }

                if (offset >= buffer.size()) {
                    throw std::runtime_error("Invalid standalone bytecode payload");
                }

                Value operand;
                const std::uint8_t value_type = buffer[offset++];
                if (value_type == 0) {
                    const std::uint64_t number_bits = read_uint64(buffer, offset);
                    double number = 0;
                    std::memcpy(&number, &number_bits, sizeof(number));
                    operand = Value(number);
                } else if (value_type == 1) {
                    operand = Value(read_string(buffer, offset));
                } else if (value_type == 2) {
                    operand = Value::function(read_string(buffer, offset));
                } else {
                    throw std::runtime_error("Invalid standalone bytecode value type");
                }

                bytecode.emplace_back(static_cast<Opcode>(opcode), std::move(operand));
            }

            if (offset != buffer.size()) {
                throw std::runtime_error("Invalid standalone bytecode payload");
            }
            return bytecode;
        }

#ifdef _WIN32
        bool launched_from_explorer() {
            HANDLE process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (process_snapshot == INVALID_HANDLE_VALUE) return false;

            DWORD parent_process_id = 0;
            PROCESSENTRY32W process_entry{};
            process_entry.dwSize = sizeof(process_entry);
            if (Process32FirstW(process_snapshot, &process_entry)) {
                do {
                    if (process_entry.th32ProcessID == GetCurrentProcessId()) {
                        parent_process_id = process_entry.th32ParentProcessID;
                        break;
                    }
                } while (Process32NextW(process_snapshot, &process_entry));
            }
            CloseHandle(process_snapshot);
            if (parent_process_id == 0) return false;

            HANDLE parent_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parent_process_id);
            if (!parent_process) return false;

            std::vector<wchar_t> parent_path(32768);
            DWORD path_length = static_cast<DWORD>(parent_path.size());
            const BOOL path_found = QueryFullProcessImageNameW(
                parent_process, 0, parent_path.data(), &path_length);
            CloseHandle(parent_process);
            if (!path_found) return false;

            const std::wstring parent_name = std::filesystem::path(
                std::wstring(parent_path.data(), path_length)).filename().wstring();
            return _wcsicmp(parent_name.c_str(), L"explorer.exe") == 0;
        }
#endif
    }

    std::filesystem::path current_executable_path(const char* argv_zero) {
#ifdef _WIN32
        std::vector<wchar_t> path_buffer(32768);
        const DWORD path_length = GetModuleFileNameW(
            nullptr, path_buffer.data(), static_cast<DWORD>(path_buffer.size()));
        if (path_length == 0 || path_length >= path_buffer.size()) {
            throw std::runtime_error("Couldn't determine executable path");
        }
        return std::filesystem::path(std::wstring(path_buffer.data(), path_length));
#else
        std::error_code error;
        const auto executable_path = std::filesystem::read_symlink("/proc/self/exe", error);
        if (!error) return executable_path;
        return std::filesystem::absolute(argv_zero ? argv_zero : "");
#endif
    }

    std::optional<Compiler::Bytecode> read_embedded_bytecode(const std::filesystem::path& executable_path) {
        std::ifstream executable(executable_path, std::ios::binary);
        if (!executable.is_open()) {
            throw std::runtime_error("Couldn't read executable: " + executable_path.string());
        }

        executable.seekg(0, std::ios::end);
        const std::streamoff file_size = executable.tellg();
        if (file_size < static_cast<std::streamoff>(standalone_trailer_size)) return std::nullopt;

        std::array<std::uint8_t, standalone_trailer_size> trailer{};
        executable.seekg(file_size - static_cast<std::streamoff>(trailer.size()));
        executable.read(reinterpret_cast<char*>(trailer.data()), static_cast<std::streamsize>(trailer.size()));
        if (!executable || std::memcmp(trailer.data() + sizeof(std::uint64_t),
                                      standalone_magic.data(), standalone_magic.size()) != 0) {
            return std::nullopt;
        }

        const ByteBuffer trailer_buffer(trailer.begin(), trailer.end());
        std::size_t trailer_offset = 0;
        const std::uint64_t payload_size = read_uint64(trailer_buffer, trailer_offset);
        const std::streamoff payload_limit = file_size - static_cast<std::streamoff>(trailer.size());
        if (payload_size > static_cast<std::uint64_t>(payload_limit) ||
            payload_size > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("Invalid standalone bytecode payload");
        }

        executable.seekg(payload_limit - static_cast<std::streamoff>(payload_size));
        ByteBuffer payload(static_cast<std::size_t>(payload_size));
        executable.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
        if (!executable && !payload.empty()) {
            throw std::runtime_error("Couldn't read standalone bytecode payload");
        }
        return deserialize_bytecode(payload);
    }

    std::filesystem::path create_standalone_executable(
        const std::filesystem::path& executable_path,
        const std::string& source_path,
        const Compiler::Bytecode& bytecode) {
        std::filesystem::path output_path(source_path);
        output_path.replace_extension(".exe");
        if (std::filesystem::exists(output_path) && std::filesystem::equivalent(output_path, executable_path)) {
            throw std::runtime_error("Standalone output cannot overwrite the Cove executable");
        }

        const ByteBuffer payload = serialize_bytecode(bytecode);
        std::filesystem::copy_file(
            executable_path, output_path, std::filesystem::copy_options::overwrite_existing);

        std::ofstream output(output_path, std::ios::binary | std::ios::app);
        if (!output.is_open()) {
            throw std::runtime_error("Couldn't write standalone executable: " + output_path.string());
        }
        output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));

        ByteBuffer trailer;
        append_uint64(trailer, static_cast<std::uint64_t>(payload.size()));
        trailer.insert(trailer.end(), standalone_magic.begin(), standalone_magic.end());
        output.write(reinterpret_cast<const char*>(trailer.data()), static_cast<std::streamsize>(trailer.size()));
        if (!output) {
            throw std::runtime_error("Couldn't finish standalone executable: " + output_path.string());
        }
        return output_path;
    }

    void pause_if_launched_from_explorer() {
#ifdef _WIN32
        if (launched_from_explorer()) {
            std::cout << "\nPress any key to close..." << std::flush;
            _getch();
            std::cout << std::endl;
        }
#endif
    }
}