#include "../machine.hpp"

namespace ops {
    void classes(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case CDEF: {
                std::string class_name = instr.operand.as_string();
                vm.cdefs[class_name] = umap<std::string, Value>();
                vm.curr_class_def = class_name;
                break;
            }
            case CNUM: {
                std::string num_var_name = instr.operand.as_string();
                vm.cdefs[vm.curr_class_def][num_var_name] = Value();
                break;
            }
            case CSTR: {
                std::string str_var_name = instr.operand.as_string();
                vm.cdefs[vm.curr_class_def][str_var_name] = Value("");
                break;
            }
            case INSTC: {
                std::string instance_name = instr.operand.as_string();
                std::string class_name = vm.stack.pop().as_string();
                auto class_it = vm.cdefs.find(class_name);
                if (class_it == vm.cdefs.end()) {
                    throw std::runtime_error("Class not found: " + class_name);
                }
                vm.cscopes.back()[instance_name] = class_it->second;
                break;
            }
            case CLOAD: {
                std::string instance_name = vm.stack.pop().as_string();
                std::string member_name = instr.operand.as_string();
                bool found_instance = false;
                for (auto it = vm.cscopes.rbegin(); it != vm.cscopes.rend(); ++it) {
                    auto instance_it = it->find(instance_name);
                    if (instance_it == it->end()) continue;
                    auto member_it = instance_it->second.find(member_name);
                    if (member_it == instance_it->second.end()) {
                        throw std::runtime_error("Member not found in class instance '" + instance_name + "': " + member_name);
                    }
                    vm.stack.push(member_it->second);
                    found_instance = true;
                    break;
                }
                if (!found_instance) {
                    throw std::runtime_error("Class instance not found: " + instance_name);
                }
                break;
            }
            case CSTORE: {
                std::string member_name = vm.stack.pop().as_string();
                std::string instance_name = vm.stack.pop().as_string();
                Value value = vm.stack.pop();
                bool found_instance = false;
                for (auto it = vm.cscopes.rbegin(); it != vm.cscopes.rend(); ++it) {
                    auto instance_it = it->find(instance_name);
                    if (instance_it == it->end()) continue;
                    auto member_it = instance_it->second.find(member_name);
                    if (member_it == instance_it->second.end()) {
                        throw std::runtime_error("Member not found in class instance '" + instance_name + "': " + member_name);
                    }
                    member_it->second = value;
                    found_instance = true;
                    break;
                }
                if (!found_instance) {
                    throw std::runtime_error("Class instance not found: " + instance_name);
                }
                break;
            }
            default:
                throw std::runtime_error("Invalid class operation");
        }
    }
}