#pragma once
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <iostream>
#include <vector>
#include "../shared/structs.hpp"

#define umap std::unordered_map

class Machine; 

namespace ops {
    void stack(Machine& vm, Instruction& instr);
    void arithmetic(Machine& vm, Instruction& instr);
    void io(Machine& vm, Instruction& instr);
    void vars(Machine& vm, Instruction& instr);
    void cflow(Machine& vm, Instruction& instr, uint64_t& next_pc);
    void tables(Machine& vm, Instruction& instr);
    void classes(Machine& vm, Instruction& instr, uint64_t& next_pc);
}

class Machine {
public:
    Stack stack;
    std::vector<umap<std::string, Value>> scopes;
    std::vector<umap<std::string, umap<std::string, Value>>> tablescopes;

    umap<std::string, umap<std::string, Value>> cdefs;
    umap<std::string, uint64_t> classdef_constructor_pos;
    umap<std::string, umap<std::string, uint64_t>> member_function_addresses;
    std::vector<umap<std::string, umap<std::string, Value>>> cscopes;
    std::vector<umap<std::string, std::string>> class_type_scopes;
    std::string curr_class_def = "";

    

    std::vector<uint64_t> call_stack;
    std::vector<bool> constructor_call_stack;
    std::vector<bool> member_function_call_stack;
    std::vector<std::string> constructor_instances;

    umap<std::string, uint64_t> function_addresses;
    uint64_t pc = 0;

    inline void run(std::vector<Instruction> bc) {
        pc = 0;
        call_stack.clear();
        constructor_call_stack.clear();
        member_function_call_stack.clear();
        constructor_instances.clear();
        function_addresses.clear();
        cdefs.clear();
        classdef_constructor_pos.clear();
        member_function_addresses.clear();

        scopes.clear();
        cscopes.clear();
        class_type_scopes.clear();
        tablescopes.clear();

        scopes.emplace_back();
        tablescopes.emplace_back();
        cscopes.emplace_back();
        class_type_scopes.emplace_back();

        for (size_t i = 0; i < bc.size(); ++i) {
            if (bc[i].op == FUNC) {
                function_addresses[bc[i].operand.as_string()] = i + 1;
            }
        }

        while (pc < bc.size()) {
            Instruction& instr = bc[pc];
            uint64_t next_pc = pc + 1;

            switch (instr.op) {
                // Stack
                case PUSH: case POP: case DUP: case SWAP:
                    ops::stack(*this, instr);
                    break;
                
                // Arithmetic 
                case ADD: case SUB: case MUL: case DIV: case MOD:
                    ops::arithmetic(*this, instr);
                    break;

                // I/O 
                case PRINT: case INP:
                    ops::io(*this, instr);
                    break;

                // Variables 
                case STORE: case LOAD:
                    ops::vars(*this, instr);
                    break;

                // Control Flow
                case EQ: case NEQ: case LT: case GT:
                case JZ: case JNZ: case JE: case JNE: case JMP:
                case FUNC: case CALL: case RET:
                    ops::cflow(*this, instr, next_pc);
                    break;

                // Tables 
                case CT: case STV: case LTV:
                    ops::tables(*this, instr);
                    break;

                // Classes
                case CDEF: case CNUM: case CSTR: case INSTC: case CLOAD: case CSTORE: case CONSTRUCTOR: case CCONSTRUCTOR: case MFUNC: case CMFUNC:
                    ops::classes(*this, instr, next_pc);
                    break;
            }

            pc = next_pc;
        }
    }
};