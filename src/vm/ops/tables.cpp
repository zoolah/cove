#include "../machine.hpp"

namespace ops {
    void tables(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case CT: {
                std::string tablename = instr.operand.as_string();
                auto& current = vm.tablescopes.back();
                if (current.find(tablename) != current.end()) {
                    throw std::runtime_error("Attempt to redefine table: " + tablename);
                }
                current[tablename] = {};
                break;
            }
            case STV: {
                Value value = vm.stack.pop();
                std::string tablename = vm.stack.pop().as_string();
                std::string key = instr.operand.as_string();

                bool found = false;
                for (auto it = vm.tablescopes.rbegin(); it != vm.tablescopes.rend(); ++it) {
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
            case LTV: {
                std::string tablename = vm.stack.pop().as_string();
                std::string key = instr.operand.as_string();

                bool found = false;
                for (auto it = vm.tablescopes.rbegin(); it != vm.tablescopes.rend(); ++it) {
                    auto table_it = it->find(tablename);
                    if (table_it != it->end()) {
                        auto& table = table_it->second;
                        if (table.find(key) == table.end()) {
                            throw std::runtime_error("Key not found in table '" + tablename + "': " + key);
                        }
                        vm.stack.push(table[key]);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    throw std::runtime_error("Table not found: " + tablename);
                }
                break;
            }
            default:
                throw std::runtime_error("Invalid table operation");
        }
    }
}