#include "../compiler.hpp"

void Compiler::print_bytecode(const std::vector<Instruction>& bytecode) {
        int indent = 0;
        bool in_function = false;

        auto format_number = [](double value) {
            std::ostringstream oss;
            oss << value;
            std::string text = oss.str();
            if (text.find('.') != std::string::npos) {
                while (!text.empty() && text.back() == '0') text.pop_back();
                if (!text.empty() && text.back() == '.') text.pop_back();
            }
            return text;
        };

        auto format_operand = [&](const Value& operand) {
            if (operand.is_number()) {
                return format_number(operand.as_number());
            }
            if (operand.is_function()) {
                return "fn:" + operand.as_function();
            }
            return operand.as_string();
        };

        for (size_t i = 0; i < bytecode.size(); ++i) {
            const auto& instr = bytecode[i];
            std::string label;
            std::string operand_text;
            bool has_operand = false;

            switch (instr.op) {
            case PUSH:   label = "PUSH"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case POP:    label = "POP"; break;
            case DUP:    label = "DUP"; break;
            case SWAP:   label = "SWAP"; break;
            case ADD:    label = "ADD"; break;
            case SUB:    label = "SUB"; break;
            case MUL:    label = "MUL"; break;
            case DIV:    label = "DIV"; break;
            case MOD:    label = "MOD"; break;
            case PRINT:  label = "PRINT"; break;
            case STORE:  label = "STORE"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case LOAD:   label = "LOAD"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case EQ:     label = "EQ"; break;
            case LT:     label = "LT"; break;
            case GT:     label = "GT"; break;
            case NEQ:    label = "NEQ"; break;
            case JZ:     label = "JZ"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case JNZ:    label = "JNZ"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case JNE:    label = "JNE"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case JE:     label = "JE"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case JMP:    label = "JMP"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case CONCAT: label = "CONCAT"; break;
            case CT:     label = "CT"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case STV:    label = "STV"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case LTV:    label = "LTV"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case FUNC:   label = "FUNC"; operand_text = instr.operand.as_string(); has_operand = true; indent = 2; in_function = true; break;
            case CALL:   label = "CALL"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case RET:    label = "RET"; break;
            case CDEF:   label = "CDEF"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case CNUM:   label = "CNUM"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case CSTR:   label = "CSTR"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case INSTC: label = "INSTC"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case CLOAD:  label = "CLOAD"; operand_text = instr.operand.as_string(); has_operand = true; break;
            case CSTORE: label = "CSTORE"; break;
            default:     label = "UNKNOWN"; break;
            }

            bool implicit_return_tail = in_function && instr.op == RET &&
                i + 2 < bytecode.size() &&
                bytecode[i + 1].op == PUSH &&
                bytecode[i + 1].operand.is_number() &&
                bytecode[i + 1].operand.as_number() == 0.0 &&
                bytecode[i + 2].op == RET;

            std::cout << std::string(indent, ' ')
                      << std::setw(2) << i << ": "
                      << std::left << std::setw(8) << label;
            if (has_operand) {
                std::cout << " " << operand_text;
            }
            std::cout << "\n";

            if (instr.op == RET) {
                if (!implicit_return_tail) {
                    indent = 0;
                    in_function = false;
                }
            }
        }
}