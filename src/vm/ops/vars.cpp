#include "../machine.hpp"

namespace ops {
    void vars(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case STORE: {
                Value val = vm.stack.pop();
                vm.scopes.back()[instr.operand.as_string()] = val;
                break;
            }
            case LOAD: {
                bool found = false;
                for (auto it = vm.scopes.rbegin(); it != vm.scopes.rend(); ++it) {
                    auto var = it->find(instr.operand.as_string());
                    if (var != it->end()) {
                        vm.stack.push(var->second);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto func_it = vm.function_addresses.find(instr.operand.as_string());
                    if (func_it != vm.function_addresses.end()) {
                        vm.stack.push(Value::function(instr.operand.as_string()));
                        found = true;
                    }
                }
                if (!found) {
                    throw std::runtime_error("Undefined variable: " + instr.operand.as_string());
                }
                break;
            }
            default:
                throw std::runtime_error("Invalid variable operation");
        }
    }
}