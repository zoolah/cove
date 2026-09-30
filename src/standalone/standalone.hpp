#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include "../compiler/compiler.hpp"

namespace Standalone {
    std::filesystem::path current_executable_path(const char* argv_zero);
    std::optional<Compiler::Bytecode> read_embedded_bytecode(const std::filesystem::path& executable_path);
    std::filesystem::path create_standalone_executable(
        const std::filesystem::path& executable_path,
        const std::string& source_path,
        const Compiler::Bytecode& bytecode);
    void pause_if_launched_from_explorer();
}