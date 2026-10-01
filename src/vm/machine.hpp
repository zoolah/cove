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

enum class CallType {
    Function,
    Constructor,
    MemberFunction
};

struct CallFrame {
    uint64_t return_pc;
    CallType type;
    std::string instance_name;
};

class Machine {
public:
    Stack stack;

    // Scoping
    std::vector<umap<std::string, Value>> scopes;
    std::vector<umap<std::string, umap<std::string, Value>>> tablescopes;
    std::vector<umap<std::string, umap<std::string, Value>>> cscopes;
    std::vector<umap<std::string, std::string>> class_type_scopes;


    std::vector<CallFrame> call_frames;
    umap<std::string, umap<std::string, Value>> cdefs;
    std::string curr_class_def = "";


    // function addresses
    umap<std::string, uint64_t> function_addresses;
    umap<std::string, uint64_t> classdef_constructor_pos;
    umap<std::string, umap<std::string, uint64_t>> member_function_addresses;
    
    uint64_t pc = 0;

    inline void push_scope() {
        scopes.emplace_back();
        tablescopes.emplace_back();
        cscopes.emplace_back();
        class_type_scopes.emplace_back();
    }

    inline void pop_scope() {
        if (scopes.size() > 1) scopes.pop_back();
        if (tablescopes.size() > 1) tablescopes.pop_back();
        if (cscopes.size() > 1) cscopes.pop_back();
        if (class_type_scopes.size() > 1) class_type_scopes.pop_back();
    }

    inline void enter_call(uint64_t return_pc, CallType type = CallType::Function, const std::string& instance_name = "") {
        push_scope();
        call_frames.push_back({ return_pc, type, instance_name });
    }

    inline CallFrame leave_call() {
        if (call_frames.empty()) {
            throw std::runtime_error("Call stack underflow on RET");
        }
        CallFrame frame = call_frames.back();
        call_frames.pop_back();
        pop_scope();
        return frame;
    }

    inline std::string current_instance_name() const {
        for (auto frame = call_frames.rbegin(); frame != call_frames.rend(); ++frame) {
            if (frame->type != CallType::Function) return frame->instance_name;
        }
        return "";
    }

    inline void run(std::vector<Instruction> bc) {
        pc = 0;
        call_frames.clear();
        function_addresses.clear();
        cdefs.clear();
        classdef_constructor_pos.clear();
        member_function_addresses.clear();

        scopes.clear();
        cscopes.clear();
        class_type_scopes.clear();
        tablescopes.clear();

        push_scope();

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