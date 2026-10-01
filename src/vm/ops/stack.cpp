#include "../machine.hpp"

namespace ops {
    void stack(Machine& vm, Instruction& instr) {
        switch (instr.op) {
            case PUSH:
                vm.stack.push(instr.operand);
                break;
            case POP:
                vm.stack.pop();
                break;
            case DUP: {
                Value v = vm.stack.pop();   
                vm.stack.push(v);
                vm.stack.push(v);
                break;
            }
            case SWAP:
                if (vm.stack.size() < 2)
                    throw std::runtime_error("SWAP requires two values");
                {
                    Value a = vm.stack.pop();
                    Value b = vm.stack.pop();
                    vm.stack.push(a);
                    vm.stack.push(b);
                }
                break;
            default:
                throw std::runtime_error("Invalid stack operation");
        }
    }
}