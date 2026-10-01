#include "../machine.hpp"

namespace ops {
    static std::string resolve_instance_name(Machine& vm, const std::string& name) {
        if (name != "this") return name;
        const std::string instance_name = vm.current_instance_name();
        if (instance_name.empty()) {
            throw std::runtime_error("'this' can only be used inside a class constructor");
        }
        return instance_name;
    }

    void classes(Machine& vm, Instruction& instr, uint64_t& next_pc) {
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
                vm.class_type_scopes.back()[instance_name] = class_name;
                break;
            }
            case CLOAD: {
                std::string instance_name = resolve_instance_name(vm, vm.stack.pop().as_string());
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
                std::string instance_name = resolve_instance_name(vm, vm.stack.pop().as_string());
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
            case CONSTRUCTOR: {
                vm.classdef_constructor_pos[vm.curr_class_def] = vm.pc + 2; // CONSTRUCTOR, +1 (JMP TO AFTER), +2 (START OF FUNC BODY)
                break;
            }
            case MFUNC: {
                const std::string func_name = instr.operand.as_string();
                vm.member_function_addresses[vm.curr_class_def][func_name] = vm.pc + 2; 
                break;
            }
            case CMFUNC: {
                const std::string instance_name = resolve_instance_name(vm, vm.stack.pop().as_string());
                std::string class_name;
                bool found_instance = false;
                for (auto it = vm.class_type_scopes.rbegin(); it != vm.class_type_scopes.rend(); ++it) {
                    auto instance_it = it->find(instance_name);
                    if (instance_it == it->end()) continue;
                    class_name = instance_it->second;
                    found_instance = true;
                    break;
                }
                if (!found_instance) {
                    throw std::runtime_error("Class instance not found: " + instance_name);
                }
                const std::string func_name = instr.operand.as_string();
                auto class_it = vm.member_function_addresses.find(class_name);
                if (class_it == vm.member_function_addresses.end() || class_it->second.find(func_name) == class_it->second.end()) {
                    throw std::runtime_error("Member function not defined for class '" + class_name + "': " + func_name);
                }
                vm.enter_call(next_pc, CallType::MemberFunction, instance_name);
                next_pc = class_it->second[func_name];
                break;
            }
            case CCONSTRUCTOR: {
                const std::string instance_name = vm.stack.pop().as_string();
                const std::string class_name = instr.operand.as_string();
                auto constructor_it = vm.classdef_constructor_pos.find(class_name);
                if (constructor_it == vm.classdef_constructor_pos.end()) {
                    throw std::runtime_error("Constructor not defined for class '" + class_name + "'");
                }
                vm.enter_call(next_pc, CallType::Constructor, instance_name);
                next_pc = constructor_it->second;
                break;
            }
            default:
                throw std::runtime_error("Invalid class operation");
        }
    }
}