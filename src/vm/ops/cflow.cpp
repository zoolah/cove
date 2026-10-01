#include "../machine.hpp"

namespace ops {
    void cflow(Machine& vm, Instruction& instr, uint64_t& next_pc) {
        switch (instr.op) {
            case EQ: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (a.is_number() && b.is_number()) {
                    vm.stack.push(Value(a.as_number() == b.as_number() ? 1.0 : 0.0));
                } else if (a.is_string() && b.is_string()) {
                    vm.stack.push(Value(a.as_string() == b.as_string() ? 1.0 : 0.0));
                } else {
                    throw std::runtime_error("Cannot EQ number and string");
                }
                break;
            }
            case NEQ: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (a.is_number() && b.is_number()) {
                    vm.stack.push(Value(a.as_number() != b.as_number() ? 1.0 : 0.0));
                } else if (a.is_string() && b.is_string()) {
                    vm.stack.push(Value(a.as_string() != b.as_string() ? 1.0 : 0.0));
                } else {
                    throw std::runtime_error("Cannot NEQ number and string");
                }
                break;
            }
            case LT: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (!a.is_number() || !b.is_number()) {
                    throw std::runtime_error("Error: Attempt to compare non-number with <");
                }
                vm.stack.push(Value(b.as_number() < a.as_number() ? 1.0 : 0.0));
                break;
            }
            case GT: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (!a.is_number() || !b.is_number()) {
                    throw std::runtime_error("Error: Attempt to compare non-number with >");
                }
                vm.stack.push(Value(b.as_number() > a.as_number() ? 1.0 : 0.0));
                break;
            }
            case JZ: {
                Value condition_value = vm.stack.pop();
                if (!condition_value.is_number()) {
                    throw std::runtime_error("JZ requires a number condition");
                }
                if (condition_value.as_number() == 0.0) {
                    next_pc = (uint64_t)instr.operand.as_number();
                }
                break;
            }
            case JNZ: {
                Value condition_value = vm.stack.pop();
                if (!condition_value.is_number()) {
                    throw std::runtime_error("JNZ requires a number condition");
                }
                if (condition_value.as_number() != 0.0) {
                    next_pc = (uint64_t)instr.operand.as_number();
                }
                break;
            }
            case JE: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (a.is_number() && b.is_number()) {
                    if (a.as_number() == b.as_number()) next_pc = (uint64_t)instr.operand.as_number();
                } else if (a.is_string() && b.is_string()) {
                    if (a.as_string() == b.as_string()) next_pc = (uint64_t)instr.operand.as_number();
                } else {
                    throw std::runtime_error("Cannot JE number and string");
                }
                break;
            }
            case JNE: {
                Value a = vm.stack.pop();
                Value b = vm.stack.pop();
                if (a.is_number() && b.is_number()) {
                    if (a.as_number() != b.as_number()) next_pc = (uint64_t)instr.operand.as_number();
                } else if (a.is_string() && b.is_string()) {
                    if (a.as_string() != b.as_string()) next_pc = (uint64_t)instr.operand.as_number();
                } else {
                    throw std::runtime_error("Cannot JNE number and string");
                }
                break;
            }
            case JMP: {
                next_pc = (uint64_t)instr.operand.as_number();
                break;
            }
            case FUNC: {
                //marker
                break;
            }
            case CALL: {
                std::string callee_name;
                if (instr.operand.is_string() && !instr.operand.as_string().empty()) {
                    callee_name = instr.operand.as_string();
                } else {
                    Value callee = vm.stack.pop();
                    if (callee.is_function()) callee_name = callee.as_function();
                    else if (callee.is_string()) callee_name = callee.as_string();
                    else throw std::runtime_error("Attempt to call a non-function value");
                }

                auto it = vm.function_addresses.find(callee_name);
                if (it == vm.function_addresses.end()) {
                    throw std::runtime_error("Undefined function: " + callee_name);
                }

                vm.enter_call(next_pc);
                next_pc = it->second;
                break;
            }
            case RET: {
                Value result = vm.stack.pop();
                const CallFrame frame = vm.leave_call();
                next_pc = frame.return_pc;
                if (frame.type != CallType::Constructor) vm.stack.push(result);
                break;
            }
            default:
                throw std::runtime_error("Invalid control flow operation");
        }
    }
}