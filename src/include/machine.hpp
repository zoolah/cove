#pragma once
#include <sstream>
#include <unordered_map>
#include "structs.hpp"

class Machine {
private:
    Stack stack;
    std::unordered_map<std::string, sv> variables;
    std::unordered_map<std::string, std::unordered_map<std::string, sv>> tables;
    uint64_t pc = 0;

public:
    void run(std::vector<Instruction> bc) {
        pc = 0;

        while (pc < bc.size()) {
            Instruction& instr = bc[pc];
            uint64_t next_pc = pc + 1;


            switch (instr.op) {
                case PUSH: {
                    stack.push(instr.operand);
                    break;
                }
                case ADD: {
                    sv A = stack.pop();
                    if (A.type != ValueType::NUMBER) throw std::runtime_error("ADD requires numbers");

                    sv B = stack.pop();
                    if (B.type != ValueType::NUMBER) throw std::runtime_error("ADD requires numbers");

                    stack.push(sv(B.num + A.num));
                    break;
                }
                case SUB: {
                    sv A = stack.pop();
                    if (A.type != ValueType::NUMBER) throw std::runtime_error("SUB requires numbers");

                    sv B = stack.pop();
                    if (B.type != ValueType::NUMBER) throw std::runtime_error("SUB requires numbers");

                    stack.push(sv(B.num - A.num));
                    break;
                }
                case MUL: {
                    sv A = stack.pop();
                    if (A.type != ValueType::NUMBER) throw std::runtime_error("MUL requires numbers");

                    sv B = stack.pop();
                    if (B.type != ValueType::NUMBER) throw std::runtime_error("MUL requires numbers");

                    stack.push(sv(B.num * A.num));
                    break;
                }
                case DIV: {
                    sv A = stack.pop();
                    if (A.type != ValueType::NUMBER) throw std::runtime_error("DIV requires numbers");

                    sv B = stack.pop();
                    if (B.type != ValueType::NUMBER) throw std::runtime_error("DIV requires numbers");

                    if (A.num == 0) throw std::runtime_error("Division by zero");

                    stack.push(sv(B.num / A.num));
                    break;
                }
                case MOD: {
                    sv A = stack.pop();
                    if (A.type != ValueType::NUMBER) throw std::runtime_error("MOD requires numbers");

                    sv B = stack.pop();
                    if (B.type != ValueType::NUMBER) throw std::runtime_error("MOD requires numbers");

                    double remainder = std::fmod(B.num, A.num);

                    stack.push(remainder);
                    break;

                }
                case PRINT: {
                    sv v = stack.pop();
                    if (v.type == ValueType::NUMBER) {
                        std::cout << v.num << std::endl;
                    }
                    else if (v.type == ValueType::STRING) {
                        std::cout << v.str << std::endl;
                    }
                    break;
                }
                case STORE: {
                    sv val = stack.pop();
                    variables[instr.operand.str] = val;
                    break;
                }
                case LOAD: {
                    auto it = variables.find(instr.operand.str);
                    if (it == variables.end()) {
                        throw std::runtime_error("Undefined variable: " + instr.operand.str);
                    }
                    stack.push(it->second);
                    break;
                }
                case EQ: {
                    sv a = stack.pop();
                    sv b = stack.pop();

                    if (a.type == ValueType::NUMBER && b.type == ValueType::NUMBER) {
                        if (a.num == b.num) {
                            stack.push(1.0);
                        }
                        else {
                            stack.push(0.0); 
                        }
                    }
                    else if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                        if (a.str == b.str) {
                            stack.push(1.0);
                        }
                        else {
                            stack.push(0.0); 
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot EQ number and string");
                    }
                    break;
                }
                case NEQ: {
                    sv a = stack.pop();
                    sv b = stack.pop();

                    if (a.type == ValueType::NUMBER && b.type == ValueType::NUMBER) {
                        if (a.num == b.num) {
                            stack.push(0.0);
                        }
                        else {
                            stack.push(1.0);
                        }
                    }
                    else if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                        if (a.str == b.str) {
                            stack.push(0.0);
                        }
                        else {
                            stack.push(1.0);
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot NEQ number and string");
                    }
                    break;
                }
                case LT: {
                    sv a = stack.pop();
                    sv b = stack.pop();
                    if (a.type != ValueType::NUMBER || b.type != ValueType::NUMBER) {
                        throw std::runtime_error("Error: Attempt to compare non-number with <");
                    }

                    // b is lower in the stack so its the first one
                    // if b < a 
                    if (b.num < a.num) {
                        stack.push(1.0);
                    }
                    else {
                        stack.push(0.0);
                    }

                    break;

                }
                case GT: {
                    sv a = stack.pop();
                    sv b = stack.pop();
                    if (a.type != ValueType::NUMBER || b.type != ValueType::NUMBER) {
                        throw std::runtime_error("Error: Attempt to compare non-number with >");
                    }
                    // if b > a

                    if (b.num > a.num) {
                        stack.push(1.0);
                    }
                    else {
                        stack.push(0.0);
                    }
                    break;
                }
                case JZ: {
                    sv condition_value = stack.pop();
                    if (condition_value.type != ValueType::NUMBER) {
                        throw std::runtime_error("JZ requires a number condition");
                    }

                    if (condition_value.num == 0.0) {
                        next_pc = (uint64_t)instr.operand.num; 
                    }
                    break;
                }
                case JNZ: {
                    sv condition_value = stack.pop();
                    if (condition_value.type != ValueType::NUMBER) {
                        throw std::runtime_error("JNZ requires a number condition");
                    }

                    if (condition_value.num != 0.0) {
                        next_pc = (uint64_t)instr.operand.num;
                    }
                    break;
                }
                case JE: {
                    sv a = stack.pop();
                    sv b = stack.pop();

                    if (a.type == ValueType::NUMBER && b.type == ValueType::NUMBER) {
                        if (a.num == b.num) {
                            next_pc = (uint64_t)instr.operand.num;
                        }
                    }
                    else if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                        if (a.str == b.str) {
                            next_pc = (uint64_t)instr.operand.num;
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot JE number and string");
                    }
                    break;

                }
                case JNE: {
                    sv a = stack.pop();
                    sv b = stack.pop();

                    if (a.type == ValueType::NUMBER && b.type == ValueType::NUMBER) {
                        if (a.num != b.num) {
                            next_pc = (uint64_t)instr.operand.num;
                        }
                    }
                    else if (a.type == ValueType::STRING && b.type == ValueType::STRING) {
                        if (a.str != b.str) {
                            next_pc = (uint64_t)instr.operand.num;
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot JNE number and string");
                    }
                    break;

                }
                case CONCAT: {
                    sv A = stack.pop();
                    sv B = stack.pop();

                    if (A.type != ValueType::STRING || B.type != ValueType::STRING) {
                        throw std::runtime_error("Concatenation (..) requires string operands");
                    }

                    stack.push(sv(B.str + A.str));
                    break;
                }
                case CT: { // - create table     : creates an empty table with name of operand name
                    std::string tablename = instr.operand.str;
                    if (tables.find(tablename) != tables.end()) {
                        throw std::runtime_error("Attempt to redefine table: " + tablename);
                    }
                     
                    tables[tablename] = {};

                    break;
                }
                case STV: { // - set table value : pops tablename from stack -> a, pops value from stack -> b, sets table key (from operand) to value
                    sv value = stack.pop();                
                    std::string tablename = stack.pop().str; 


                    std::string key = instr.operand.str;

                    tables[tablename][key] = value;
                    break;
                }
                case LTV: {  // - load table value : pops tablename from stack -> a, gets key from operand, pushes table.key to stack
                    std::string tablename = stack.pop().str;
                    std::string key = instr.operand.str;

                    auto table_it = tables.find(tablename);
                    if (table_it == tables.end()) {
                        throw std::runtime_error("Table not found: " + tablename);
                    }
                    if (table_it->second.find(key) == table_it->second.end()) {
                        throw std::runtime_error("Key not found in table '" + tablename + "': " + key);
                    }

                    stack.push(table_it->second[key]);
                    break;
                }


              


            }

            pc = next_pc;


        }
    }
};