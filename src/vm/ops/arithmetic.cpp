#include "../machine.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ops {
    void arithmetic(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case ADD: {
                Value A = vm.stack.pop();
                if (!A.is_number()) throw std::runtime_error("ADD requires numbers");
                Value B = vm.stack.pop();
                if (!B.is_number()) throw std::runtime_error("ADD requires numbers");
                vm.stack.push(Value(B.as_number() + A.as_number()));
                break;
            }
            case SUB: {
                Value A = vm.stack.pop();
                if (!A.is_number()) throw std::runtime_error("SUB requires numbers");
                Value B = vm.stack.pop();
                if (!B.is_number()) throw std::runtime_error("SUB requires numbers");
                vm.stack.push(Value(B.as_number() - A.as_number()));
                break;
            }
            case MUL: {
                Value A = vm.stack.pop();
                if (!A.is_number()) throw std::runtime_error("MUL requires numbers");
                Value B = vm.stack.pop();
                if (!B.is_number()) throw std::runtime_error("MUL requires numbers");
                vm.stack.push(Value(B.as_number() * A.as_number()));
                break;
            }
            case DIV: {
                Value A = vm.stack.pop();
                if (!A.is_number()) throw std::runtime_error("DIV requires numbers");
                Value B = vm.stack.pop();
                if (!B.is_number()) throw std::runtime_error("DIV requires numbers");
                if (A.as_number() == 0) throw std::runtime_error("Division by zero");
                vm.stack.push(Value(B.as_number() / A.as_number()));
                break;
            }
            case MOD: {
                Value A = vm.stack.pop();
                if (!A.is_number()) throw std::runtime_error("MOD requires numbers");
                Value B = vm.stack.pop();
                if (!B.is_number()) throw std::runtime_error("MOD requires numbers");
                double remainder = std::fmod(B.as_number(), A.as_number());
                vm.stack.push(Value(remainder));
                break;
            }
            case CONCAT: {
                Value A = vm.stack.pop();
                Value B = vm.stack.pop();

                auto to_string = [](Value value) -> std::string {
                    if (value.is_string()) return value.as_string();
                    if (value.is_number()) {
                        if (std::trunc(value.as_number()) == value.as_number()) {
                            return std::to_string(static_cast<long long>(value.as_number()));
                        }
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(15) << value.as_number();
                        std::string s = oss.str();
                        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                        if (!s.empty() && s.back() == '.') s.pop_back();
                        return s;
                    }
                    throw std::runtime_error("Concatenation (..) requires string or number operands");
                };

                vm.stack.push(Value(to_string(B) + to_string(A)));
                break;
            }
            default:
                throw std::runtime_error("Invalid arithmetic operation");
        }
    }
}