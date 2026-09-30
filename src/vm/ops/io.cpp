#include "../machine.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

namespace ops {
    void io(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case PRINT: {
                Value v = vm.stack.pop();
                if (v.is_number()) {
                    if (std::trunc(v.as_number()) == v.as_number()) {
                        std::cout << static_cast<long long>(v.as_number()) << std::endl;
                    } else {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(15) << v.as_number();
                        std::string s = oss.str();
                        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                        if (!s.empty() && s.back() == '.') s.pop_back();
                        std::cout << s << std::endl;
                    }
                } else if (v.is_string()) {
                    std::cout << v.as_string() << std::endl;
                }
                break;
            }
            case INP: {
                Value argument = vm.stack.pop();
                std::cout << argument.as_string();
                std::string buf;
                std::cin >> buf;
                vm.stack.push(Value(buf));
                break;
            }
            default:
                throw std::runtime_error("Invalid I/O operation");
        }
    }
}