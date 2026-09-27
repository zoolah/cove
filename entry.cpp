#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include "machine.hpp"
#include "compiler.hpp"
#include "tokenizer.hpp"


#define debug false

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf(); 
    return buffer.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: cove.exe <filename.txt>" << std::endl;
        return 1;
    }

    try {
        std::string source_code = read_file(argv[1]);

        auto start = std::chrono::steady_clock::now();
        std::vector<Token> tokenized = Tokenizer::tokenize(source_code);
        std::vector<Instruction> bytecode = Compiler::compile(tokenized);

        if (debug)
            Compiler::print_bytecode(bytecode);


        auto end = std::chrono::steady_clock::now();

        auto elapsed = end - start;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();

        if (debug)
            std::cout << std::endl << "Compile took " << elapsed_ms << " ms\n";


        if (bytecode.size() == 0) {
            throw std::runtime_error("Compilation failed");
        }

        Machine vm;
        vm.run(bytecode);


    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
     
    return 0;
}