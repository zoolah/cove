#pragma once
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <iostream>
#include "structs.hpp"

class Machine {
private:
    Stack stack;
    std::vector<std::unordered_map<std::string, sv>> scopes;  // variables in scopes
    std::vector < std::unordered_map<std::string, std::unordered_map<std::string, sv>>> tablescopes; // tables in scopes

    std::vector<uint64_t> call_stack; // return addresses
    std::unordered_map<std::string, uint64_t> function_addresses; // maps func names to their entry points
    uint64_t pc = 0;

public:
    void run(std::vector<Instruction> bc) {
        pc = 0;
        call_stack.clear();
        function_addresses.clear();
        scopes.clear();
        tablescopes.clear();         

        scopes.emplace_back();        
        tablescopes.emplace_back();   
         
        for (size_t i = 0; i < bc.size(); ++i) {
            if (bc[i].op == FUNC) {
                function_addresses[bc[i].operand.str] = i + 1;  // map each function start operator with its name to its address
            }
        }

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
                        if (std::trunc(v.num) == v.num) {
                            std::cout << static_cast<long long>(v.num) << std::endl;
                        }
                        else {
                            std::cout << v.num << std::endl;
                        }
                    }
                    else if (v.type == ValueType::STRING) {
                        std::cout << v.str << std::endl;
                    }
                    break;
                }
                case STORE: {
                    sv val = stack.pop();
                    scopes.back()[instr.operand.str] = val; // store the value in the current scope (lowest)
                    break;
                }
                case LOAD: {
                    bool found = false;
                    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) { // search for variables to load from innermost to outermost, cant go deeper.
                        auto var = it->find(instr.operand.str);
                        if (var != it->end()) {
                            stack.push(var->second);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        auto func_it = function_addresses.find(instr.operand.str);
                        if (func_it != function_addresses.end()) {
                            stack.push(sv(instr.operand.str, ValueType::FUNCTION));
                            found = true;
                        }
                    }
                    if (!found) {
                        throw std::runtime_error("Undefined variable: " + instr.operand.str);
                    }
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
                case JMP: {
                    next_pc = (uint64_t)instr.operand.num;
                    break;
                }
                case CONCAT: {
                    sv A = stack.pop();
                    sv B = stack.pop();

                    auto to_string = [](sv value) -> std::string {
                        if (value.type == ValueType::STRING) {
                            return value.str;
                        }
                        if (value.type == ValueType::NUMBER) {
                            return std::to_string(value.num);
                        }
                        throw std::runtime_error("Concatenation (..) requires string or number operands");
                    };

                    stack.push(sv(to_string(B) + to_string(A)));
                    break;
                }
                case CT: { // - create table     : creates an empty table with name of operand name
                    std::string tablename = instr.operand.str;

                    auto& current = tablescopes.back();                // get current table scope
                    if (current.find(tablename) != current.end()) {    
                        throw std::runtime_error("Attempt to redefine table: " + tablename);
                    }
                     
                    current[tablename] = {};   

                    break;
                }
                case STV: { // - set table value : pops tablename from stack -> a, pops value from stack -> b, sets table key (from operand) to value
                    sv value = stack.pop();
                    std::string tablename = stack.pop().str;
                    std::string key = instr.operand.str;

                    bool found = false;
                    for (auto it = tablescopes.rbegin(); it != tablescopes.rend(); ++it) {
                        auto table_it = it->find(tablename);
                        if (table_it != it->end()) {
                            table_it->second[key] = value;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        throw std::runtime_error("Table not found: " + tablename);
                    }
                    break;
                }
                case LTV: {  // - load table value : pops tablename from stack -> a, gets key from operand, pushes table.key to stack
                    std::string tablename = stack.pop().str;
                    std::string key = instr.operand.str;

                    bool found = false;
                    for (auto it = tablescopes.rbegin(); it != tablescopes.rend(); ++it) {
                        auto table_it = it->find(tablename);
                        if (table_it != it->end()) {
                            auto& table = table_it->second;
                            if (table.find(key) == table.end()) {
                                throw std::runtime_error("Key not found in table '" + tablename + "': " + key);
                            }
                            stack.push(table[key]);
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        throw std::runtime_error("Table not found: " + tablename);
                    }
                    break;
                }
                case FUNC: {
                    // Marker instruction skipped by JMP
                    break;
                }
                case CALL: {
                    std::string callee_name;

                    if (!instr.operand.str.empty()) {
                        callee_name = instr.operand.str;
                    }
                    else {
                        sv callee = stack.pop();
                        if (callee.type == ValueType::FUNCTION || callee.type == ValueType::STRING) {
                            callee_name = callee.str;
                        }
                        else {
                            throw std::runtime_error("Attempt to call a non-function value");
                        }
                    }

                    auto it = function_addresses.find(callee_name);
                    if (it == function_addresses.end()) {
                        throw std::runtime_error("Undefined function: " + callee_name);
                    }

                    scopes.emplace_back();       // add new scope level (we're going 1 level deeper when we call a function)
                    tablescopes.emplace_back();  // add new scope level for tables as well
                    call_stack.push_back(next_pc);
                    next_pc = it->second;
                    break;
                }
                case RET: {
                    if (call_stack.empty()) {
                        throw std::runtime_error("Call stack underflow on RET");
                    }

                    sv result = stack.pop();

                    if (scopes.size() > 1) {
                        scopes.pop_back();      // go up one scope level 
                    }

                    if (tablescopes.size() > 1) {   
                        tablescopes.pop_back(); // same for tables
                    }

                    next_pc = call_stack.back(); // grab the most recent return address (from before func was called)
                    call_stack.pop_back();       // remove it 
                    stack.push(result);
                    break;
                }

                case INP: {
                    // compiler evaluates the argument and pushes to stack right before this
                    // so

                    sv argument = stack.pop();

                    std::cout << argument.str;

                    std::string buf;

                    std::cin >> buf;

                    // set value's num and string value so it can be used as both
                    sv val;
                    val.str = buf;
                    try {
                        val.num = std::stod(buf); 
                    }
                    catch (...) {
                        val.type = ValueType::STRING; // cant be casted to a number
                        //
                    }
                    

                    stack.push(val);
                    
                }

              


            }

            pc = next_pc;


        }
    }
};