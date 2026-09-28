#pragma once
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <iostream>
#include "structs.hpp"

class Machine {
private:
    Stack stack;
    std::vector<std::unordered_map<std::string, Value>> scopes;  // variables in scopes
    std::vector < std::unordered_map<std::string, std::unordered_map<std::string, Value>>> tablescopes; // tables in scopes

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
                function_addresses[bc[i].operand.as_string()] = i + 1;  // map each function start operator with its name to its address
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
                    Value A = stack.pop();
                    if (!A.is_number()) throw std::runtime_error("ADD requires numbers");

                    Value B = stack.pop();
                    if (!B.is_number()) throw std::runtime_error("ADD requires numbers");

                    stack.push(Value(B.as_number() + A.as_number()));
                    break;
                }
                case SUB: {
                    Value A = stack.pop();
                    if (!A.is_number()) throw std::runtime_error("SUB requires numbers");

                    Value B = stack.pop();
                    if (!B.is_number()) throw std::runtime_error("SUB requires numbers");

                    stack.push(Value(B.as_number() - A.as_number()));
                    break;
                }
                case MUL: {
                    Value A = stack.pop();
                    if (!A.is_number()) throw std::runtime_error("MUL requires numbers");

                    Value B = stack.pop();
                    if (!B.is_number()) throw std::runtime_error("MUL requires numbers");

                    stack.push(Value(B.as_number() * A.as_number()));
                    break;
                }
                case DIV: {
                    Value A = stack.pop();
                    if (!A.is_number()) throw std::runtime_error("DIV requires numbers");

                    Value B = stack.pop();
                    if (!B.is_number()) throw std::runtime_error("DIV requires numbers");

                    if (A.as_number() == 0) throw std::runtime_error("Division by zero");

                    stack.push(Value(B.as_number() / A.as_number()));
                    break;
                }
                case MOD: {
                    Value A = stack.pop();
                    if (!A.is_number()) throw std::runtime_error("MOD requires numbers");

                    Value B = stack.pop();
                    if (!B.is_number()) throw std::runtime_error("MOD requires numbers");

                    double remainder = std::fmod(B.as_number(), A.as_number());

                    stack.push(Value(remainder));
                    break;

                }
                case PRINT: {
                    Value v = stack.pop();
                    if (v.is_number()) {
                        if (std::trunc(v.as_number()) == v.as_number()) {
                            std::cout << static_cast<long long>(v.as_number()) << std::endl;
                        }
                        else {
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(15) << v.as_number();
                            std::string s = oss.str();

                            s.erase(s.find_last_not_of('0') + 1, std::string::npos);

                            if (!s.empty() && s.back() == '.') {
                                s.pop_back();
                            }

                            std::cout << s << std::endl;
                        }
                    }
                    else if (v.is_string()) {
                        std::cout << v.as_string() << std::endl;
                    }
                    break;
                }
                case STORE: {
                    Value val = stack.pop();
                    scopes.back()[instr.operand.as_string()] = val; // store the value in the current scope (lowest)
                    break;
                }
                case LOAD: {
                    bool found = false;
                    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) { // search for variables to load from innermost to outermost, cant go deeper.
                        auto var = it->find(instr.operand.as_string());
                        if (var != it->end()) {
                            stack.push(var->second);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        auto func_it = function_addresses.find(instr.operand.as_string());
                        if (func_it != function_addresses.end()) {
                            stack.push(Value::function(instr.operand.as_string()));
                            found = true;
                        }
                    }
                    if (!found) {
                        throw std::runtime_error("Undefined variable: " + instr.operand.as_string());
                    }
                    break;
                }
                case EQ: {
                    Value a = stack.pop();
                    Value b = stack.pop();

                    if (a.is_number() && b.is_number()) {
                        if (a.as_number() == b.as_number()) {
                            stack.push(Value(0.0));
                        }
                        else {
                            stack.push(Value(1.0));
                        }
                    }
                    else if (a.is_string() && b.is_string()) {
                        if (a.as_string() == b.as_string()) {
                            stack.push(Value(0.0));
                        }
                        else {
                            stack.push(Value(1.0));
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot EQ number and string");
                    }
                    break;
                }
                case NEQ: {
                    Value a = stack.pop();
                    Value b = stack.pop();

                    if (a.is_number() && b.is_number()) {
                        if (a.as_number() == b.as_number()) {
                            stack.push(Value(0.0));
                        }
                        else {
                            stack.push(Value(1.0));
                        }
                    }
                    else if (a.is_string() && b.is_string()) {
                        if (a.as_string() == b.as_string()) {
                            stack.push(Value(0.0));
                        }
                        else {
                            stack.push(Value(1.0));
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot NEQ number and string");
                    }
                    break;
                }
                case LT: {
                    Value a = stack.pop();
                    Value b = stack.pop();
                    if (!a.is_number() || !b.is_number()) {
                        throw std::runtime_error("Error: Attempt to compare non-number with <");
                    }

                    // b is lower in the stack so its the first one
                    // if b < a 
                    if (b.as_number() < a.as_number()) {
                        stack.push(Value(1.0));
                    }
                    else {
                        stack.push(Value(0.0));
                    }

                    break;

                }
                case GT: {
                    Value a = stack.pop();
                    Value b = stack.pop();
                    if (!a.is_number() || !b.is_number()) {
                        throw std::runtime_error("Error: Attempt to compare non-number with >");
                    }
                    // if b > a

                    if (b.as_number() > a.as_number()) {
                        stack.push(Value(1.0));
                    }
                    else {
                        stack.push(Value(0.0));
                    }
                    break;
                }
                case JZ: {
                    Value condition_value = stack.pop();
                    if (!condition_value.is_number()) {
                        throw std::runtime_error("JZ requires a number condition");
                    }

                    if (condition_value.as_number() == 0.0) {
                        next_pc = (uint64_t)instr.operand.as_number();
                    }
                    break;
                }
                case JNZ: {
                    Value condition_value = stack.pop();
                    if (!condition_value.is_number()) {
                        throw std::runtime_error("JNZ requires a number condition");
                    }

                    if (condition_value.as_number() != 0.0) {
                        next_pc = (uint64_t)instr.operand.as_number();
                    }
                    break;
                }
                case JE: {
                    Value a = stack.pop();
                    Value b = stack.pop();

                    if (a.is_number() && b.is_number()) {
                        if (a.as_number() == b.as_number()) {
                            next_pc = (uint64_t)instr.operand.as_number();
                        }
                    }
                    else if (a.is_string() && b.is_string()) {
                        if (a.as_string() == b.as_string()) {
                            next_pc = (uint64_t)instr.operand.as_number();
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot JE number and string");
                    }
                    break;

                }
                case JNE: {
                    Value a = stack.pop();
                    Value b = stack.pop();

                    if (a.is_number() && b.is_number()) {
                        if (a.as_number() != b.as_number()) {
                            next_pc = (uint64_t)instr.operand.as_number();
                        }
                    }
                    else if (a.is_string() && b.is_string()) {
                        if (a.as_string() != b.as_string()) {
                            next_pc = (uint64_t)instr.operand.as_number();
                        }
                    }
                    else {
                        throw std::runtime_error("Cannot JNE number and string");
                    }
                    break;

                }
                case JMP: {
                    next_pc = (uint64_t)instr.operand.as_number();
                    break;
                }
                case CONCAT: {
                    Value A = stack.pop();
                    Value B = stack.pop();

                    auto to_string = [](Value value) -> std::string {
                        if (value.is_string()) {
                            return value.as_string();
                        }
                        if (value.is_number()) {
                            if (std::trunc(value.as_number()) == value.as_number()) {
                                return std::to_string(static_cast<long long>(value.as_number()));
                            }

                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(15) << value.as_number();
                            std::string s = oss.str();

                            s.erase(s.find_last_not_of('0') + 1, std::string::npos);

                            if (!s.empty() && s.back() == '.') {
                                s.pop_back();
                            }

                            return s;
                        }
                        throw std::runtime_error("Concatenation (..) requires string or number operands");
                    };

                    stack.push(Value(to_string(B) + to_string(A)));
                    break;
                }
                case CT: { // - create table     : creates an empty table with name of operand name
                    std::string tablename = instr.operand.as_string();

                    auto& current = tablescopes.back();                // get current table scope
                    if (current.find(tablename) != current.end()) {    
                        throw std::runtime_error("Attempt to redefine table: " + tablename);
                    }
                     
                    current[tablename] = {};   

                    break;
                }
                case STV: { // - set table value : pops tablename from stack -> a, pops value from stack -> b, sets table key (from operand) to value
                    Value value = stack.pop();
                    std::string tablename = stack.pop().as_string();
                    std::string key = instr.operand.as_string();

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
                    std::string tablename = stack.pop().as_string();
                    std::string key = instr.operand.as_string();

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

                    if (instr.operand.is_string() && !instr.operand.as_string().empty()) {
                        callee_name = instr.operand.as_string();
                    }
                    else {
                        Value callee = stack.pop();
                        if (callee.is_function()) {
                            callee_name = callee.as_function();
                        }
                        else if (callee.is_string()) {
                            callee_name = callee.as_string();
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

                    Value result = stack.pop();

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

                    Value argument = stack.pop();

                    std::cout << argument.as_string();

                    std::string buf;

                    std::cin >> buf;

                    // set value's num and string value so it can be used as both
                    stack.push(Value(buf));
                    
                }

              


            }

            pc = next_pc;


        }
    }
};