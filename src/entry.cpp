#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <vector>
#include "compiler/compiler.hpp"
#include "vm/machine.hpp"
#include "shared/structs.hpp"
#include "standalone/standalone.hpp"

namespace Color {
    constexpr const char* reset = "\033[0m";
    constexpr const char* bold = "\033[1m";
    constexpr const char* dim = "\033[2m";
    constexpr const char* cyan = "\033[36m";
    constexpr const char* green = "\033[32m";
    constexpr const char* yellow = "\033[33m";
    constexpr const char* magenta = "\033[35m";
    constexpr const char* red = "\033[31m";
}

std::string read_file(const std::string& file_path) {
    std::ifstream file(file_path);

    if (!file.is_open()) {
        throw std::runtime_error("Couldn't open file: " + file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf(); 
    return buffer.str();
}

void print_usage(const char* exe_name) {
    std::cout << Color::bold << Color::cyan << "Usage:" << Color::reset << " " << exe_name << " [--verbose | -v] [--standalone | -s] <filename>\n";
    std::cout << "       " << exe_name << " --help | -h\n";
}

int main(int argc, char** argv) {
    std::filesystem::path executable_path;
    try {
        executable_path = Standalone::current_executable_path(argc > 0 ? argv[0] : nullptr);
        if (auto bytecode = Standalone::read_embedded_bytecode(executable_path)) {
            int exit_code = 0;
            try {
                Machine vm;
                vm.run(*bytecode);
            }
            catch (const std::exception& e) {
                std::cerr << Color::red << "Fatal Error: " << e.what() << Color::reset << std::endl;
                exit_code = 1;
            }
            Standalone::pause_if_launched_from_explorer();
            return exit_code;
        }
    }
    catch (const std::exception& e) {
        std::cerr << Color::red << "Fatal Error: " << e.what() << Color::reset << std::endl;
        return 1;
    }

    bool verbose = false;
    bool standalone = false;
    std::string source_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
        else if (arg == "--standalone" || arg == "-s") {
            standalone = true;
        }
        else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        else if (source_path.empty()) {
            source_path = arg;
        }
        else {
            std::cerr << Color::red << "Unexpected argument: " << arg << Color::reset << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (source_path.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        std::string source_code = read_file(source_path);

        auto compile_start = std::chrono::steady_clock::now();
        
        std::vector<Instruction> bytecode = Compiler::compile(source_code);
        auto compile_end = std::chrono::steady_clock::now();

        if (bytecode.size() == 0) {
            throw std::runtime_error("Compilation failed");
        }

        if (verbose) {
            std::cout << Color::bold << Color::magenta << "\n=== bytecode ===" << Color::reset << "\n";
            Compiler::print_bytecode(bytecode);
            std::cout << "\n";

            auto compile_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(compile_end - compile_start).count();
            std::cout << Color::bold << Color::green << "Compile" << Color::reset << " took " << std::fixed << std::setprecision(2) << compile_ms << " ms\n";
        }

        if (standalone) {
            const auto output_path = Standalone::create_standalone_executable(executable_path, source_path, bytecode);
            std::cout << "Created standalone executable: " << output_path.string() << std::endl;
            return 0;
        }

        auto exec_start = std::chrono::steady_clock::now();
        Machine vm;
        vm.run(bytecode);
        auto exec_end = std::chrono::steady_clock::now();

        if (verbose) {
            auto exec_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(exec_end - exec_start).count();
            std::cout << Color::bold << Color::yellow << "Execute" << Color::reset << " took " << std::fixed << std::setprecision(2) << exec_ms << " ms\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << Color::red << "Fatal Error: " << e.what() << Color::reset << std::endl;
        return 1;
    }

    return 0;
}

