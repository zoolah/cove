#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>
#include "structs.hpp"
#define isspace std::isspace
#define isletter(c) (std::isalpha(c) || (c) == '_')


namespace Compiler {

    inline void print_bytecode(const std::vector<Instruction>& bytecode) {
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

        auto format_operand = [&](const sv& operand) {
            if (operand.type == ValueType::NUMBER) {
                return format_number(operand.num);
            }
            return operand.str;
        };

        for (size_t i = 0; i < bytecode.size(); ++i) {
            const auto& instr = bytecode[i];
            std::string label;
            std::string operand_text;
            bool has_operand = false;

            switch (instr.op) {
            case PUSH:   label = "PUSH"; operand_text = format_operand(instr.operand); has_operand = true; break;
            case POP:    label = "POP"; break;
            case ADD:    label = "ADD"; break;
            case SUB:    label = "SUB"; break;
            case MUL:    label = "MUL"; break;
            case DIV:    label = "DIV"; break;
            case MOD:    label = "MOD"; break;
            case PRINT:  label = "PRINT"; break;
            case STORE:  label = "STORE"; operand_text = instr.operand.str; has_operand = true; break;
            case LOAD:   label = "LOAD"; operand_text = instr.operand.str; has_operand = true; break;
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
            case CT:     label = "CT"; operand_text = instr.operand.str; has_operand = true; break;
            case STV:    label = "STV"; operand_text = instr.operand.str; has_operand = true; break;
            case LTV:    label = "LTV"; operand_text = instr.operand.str; has_operand = true; break;
            case FUNC:   label = "FUNC"; operand_text = instr.operand.str; has_operand = true; indent = 2; in_function = true; break;
            case CALL:   label = "CALL"; operand_text = instr.operand.str; has_operand = true; break;
            case RET:    label = "RET"; break;
            default:     label = "UNKNOWN"; break;
            }

            bool implicit_return_tail = in_function && instr.op == RET &&
                i + 2 < bytecode.size() &&
                bytecode[i + 1].op == PUSH &&
                bytecode[i + 1].operand.type == ValueType::NUMBER &&
                bytecode[i + 1].operand.num == 0.0 &&
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

    inline std::vector<Instruction> evaluate_expression(const std::vector<Token>& infix_expr) {
        std::vector<Instruction> bytecode;
        std::vector<Token> rpn_tokens;
        std::vector<Token> op_stack;

        std::unordered_map<size_t, std::vector<std::vector<Token>>> call_args;

        auto precedence = [](TokenType t) {
            if (t == TOK_MUL || t == TOK_DIV || t == TOK_MOD) return 3;
            if (t == TOK_ADD || t == TOK_SUB || t == TOK_CONCAT) return 2;
            if (t == TOK_EQ || t == TOK_NOTEQ || t == TOK_LT || t == TOK_GT) return 1;
            if (t == TOK_AND) return 0;
            if (t == TOK_OR) return -1;
            return -2;
            };

        for (size_t i = 0; i < infix_expr.size(); ++i) {
            const auto& tok = infix_expr[i];

            if (i + 2 < infix_expr.size() &&
                tok.type == TOK_IDENTIFIER &&
                infix_expr[i + 1].type == TOK_DOT &&
                infix_expr[i + 2].type == TOK_IDENTIFIER) {

                std::string combined = tok.value + "." + infix_expr[i + 2].value;
                rpn_tokens.push_back(Token(TOK_IDENTIFIER, combined));
                i += 2;
            }
            else if (tok.type == TOK_IDENTIFIER &&
                i + 1 < infix_expr.size() &&
                infix_expr[i + 1].type == TOK_LP) {

                std::string funcname = tok.value;
                size_t lp = i + 1;
                size_t rp = lp + 1;
                int depth = 1;
                while (rp < infix_expr.size() && depth > 0) {
                    if (infix_expr[rp].type == TOK_LP) depth++;
                    else if (infix_expr[rp].type == TOK_RP) depth--;
                    if (depth > 0) rp++;
                }
                if (depth != 0) {
                    throw std::runtime_error("Missing closing parenthesis in function call to '" + funcname + "'");
                }

                std::vector<std::vector<Token>> args;
                size_t arg_start = lp + 1;
                if (arg_start < rp) {
                    size_t current = arg_start;
                    while (current <= rp) {
                        size_t next = current;
                        int nested = 0;
                        while (next < rp) {
                            if (infix_expr[next].type == TOK_LP) nested++;
                            else if (infix_expr[next].type == TOK_RP) nested--;
                            else if (infix_expr[next].type == TOK_COMMA && nested == 0) break;
                            next++;
                        }
                        args.emplace_back(infix_expr.begin() + current, infix_expr.begin() + next);
                        current = next + 1;
                        if (next >= rp) break;
                    }
                }

                size_t marker_index = rpn_tokens.size();
                rpn_tokens.push_back(Token(TOK_IDENTIFIER, "CALL:" + funcname));
                call_args[marker_index] = std::move(args);

                i = rp; 
            }
            else if (tok.type == TOK_NUMBER || tok.type == TOK_IDENTIFIER || tok.type == TOK_STR) {
                rpn_tokens.push_back(tok);
            }
            else if (tok.type == TOK_ADD || tok.type == TOK_SUB || tok.type == TOK_MUL ||
                tok.type == TOK_DIV || tok.type == TOK_MOD || tok.type == TOK_EQ ||
                tok.type == TOK_NOTEQ || tok.type == TOK_LT || tok.type == TOK_GT ||
                tok.type == TOK_AND || tok.type == TOK_OR || tok.type == TOK_CONCAT) {
                while (!op_stack.empty() && op_stack.back().type != TOK_LP &&
                    precedence(op_stack.back().type) >= precedence(tok.type)) {
                    rpn_tokens.push_back(op_stack.back());
                    op_stack.pop_back();
                }
                op_stack.push_back(tok);
            }
            else if (tok.type == TOK_LP) {
                op_stack.push_back(tok);
            }
            else if (tok.type == TOK_RP) {
                while (!op_stack.empty() && op_stack.back().type != TOK_LP) {
                    rpn_tokens.push_back(op_stack.back());
                    op_stack.pop_back();
                }
                if (!op_stack.empty()) op_stack.pop_back();
            }
        }
        while (!op_stack.empty()) {
            rpn_tokens.push_back(op_stack.back());
            op_stack.pop_back();
        }

        for (size_t idx = 0; idx < rpn_tokens.size(); ++idx) {
            const auto& tok = rpn_tokens[idx];

            if (tok.type == TOK_NUMBER) {
                bytecode.push_back(Instruction(PUSH, sv(std::stod(tok.value))));
            }
            else if (tok.type == TOK_STR) {
                bytecode.push_back(Instruction(PUSH, sv(tok.value)));
            }
            else if (tok.type == TOK_IDENTIFIER) {
                if (tok.value.size() > 5 && tok.value.substr(0, 5) == "CALL:") {
                    std::string funcname = tok.value.substr(5);
                    auto& args = call_args[idx];

                    for (auto it = args.rbegin(); it != args.rend(); ++it) {
                        auto arg_bc = evaluate_expression(*it);
                        bytecode.insert(bytecode.end(), arg_bc.begin(), arg_bc.end());
                    }
                    bytecode.push_back(Instruction(CALL, sv(funcname)));
                }
                else {
                    size_t dot_pos = tok.value.find('.');
                    if (dot_pos != std::string::npos) {
                        std::string table_name = tok.value.substr(0, dot_pos);
                        std::string key_name = tok.value.substr(dot_pos + 1);
                        bytecode.push_back(Instruction(PUSH, sv(table_name)));
                        bytecode.push_back(Instruction(LTV, sv(key_name)));
                    }
                    else {
                        bytecode.push_back(Instruction(LOAD, sv(tok.value)));
                    }
                }
            }
            else {
                Opcode vm_op;
                switch (tok.type) {
                case TOK_ADD:    vm_op = ADD; break;
                case TOK_SUB:    vm_op = SUB; break;
                case TOK_MUL:    vm_op = MUL; break;
                case TOK_DIV:    vm_op = DIV; break;
                case TOK_MOD:    vm_op = MOD; break;
                case TOK_EQ:     vm_op = EQ;  break;
                case TOK_LT:     vm_op = LT;  break;
                case TOK_GT:     vm_op = GT;  break;
                case TOK_NOTEQ:  vm_op = NEQ; break;
                case TOK_AND:    vm_op = MUL; break;
                case TOK_OR:     vm_op = ADD; break;
                case TOK_CONCAT: vm_op = CONCAT; break;
                default: throw std::runtime_error("Unsupported operator");
                }
                bytecode.push_back(Instruction(vm_op));
            }
        }

        return bytecode;
    }
    inline size_t compile_block(const std::vector<Token>& t, size_t pos, std::vector<Instruction>& bytecode) {
        if (pos >= t.size()) return pos;
        Token curr = t[pos];

        if (curr.type != TOK_IDENTIFIER) {
            throw std::runtime_error("Unexpected token '" + curr.value + "' at root level (index " + std::to_string(pos) + ")");
        }

        if (curr.value == "print") {
            if (pos + 3 >= t.size()) {
                throw std::runtime_error("Unexpected end of file after 'print'");
            }

            if (t[pos + 1].type != TOK_LP) {
                throw std::runtime_error("Invalid print statement syntax");
            }

            size_t depth = 1;
            size_t rp_pos = pos + 2;
            while (rp_pos < t.size() && depth > 0) {
                if (t[rp_pos].type == TOK_LP) {
                    depth++;
                }
                else if (t[rp_pos].type == TOK_RP) {
                    depth--;
                    if (depth == 0) break;
                }
                
                rp_pos++;
            }
            if (rp_pos >= t.size()) {
                throw std::runtime_error("Missing closing parenthesis for print");
            }

            if (rp_pos == pos + 3 && t[pos + 2].type == TOK_STR) {
                bytecode.push_back(Instruction(PUSH, sv(t[pos + 2].value)));
            }
            else {
                std::vector<Token> infix_expr(t.begin() + pos + 2, t.begin() + rp_pos);
                auto expr_bytecode = evaluate_expression(infix_expr);
                bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());
            }
            bytecode.push_back(Instruction(PRINT));

            size_t sc_pos = rp_pos + 1;
            if (sc_pos < t.size() && t[sc_pos].type == TOK_SC) {
                return sc_pos + 1;
            }
            else {
                throw std::runtime_error("Missing closing semicolon ';' after print statement");
            }
        }
        else if (curr.value == "num") {
            if (pos + 3 >= t.size()) {
                throw std::runtime_error("Unexpected end of file after 'num' declaration at token index " + std::to_string(pos));
            }

            if (t[pos + 1].type != TOK_IDENTIFIER) {
                throw std::runtime_error("Expected variable name after 'num', but found token type " + std::to_string(t[pos + 1].type) + " ('" + t[pos + 1].value + "')");
            }
            std::string var_name = t[pos + 1].value;

            if (t[pos + 2].type != TOK_SEQ) {
                throw std::runtime_error("Expected '=' in variable declaration for '" + var_name + "', but found '" + t[pos + 2].value + "'");
            }

            size_t sc_pos = pos + 3;
            while (sc_pos < t.size() && t[sc_pos].type != TOK_SC) {
                sc_pos++;
            }
            if (sc_pos >= t.size()) {
                throw std::runtime_error("Missing closing semicolon ';' for variable declaration of '" + var_name + "'");
            }

            std::vector<Token> infix_expr(t.begin() + pos + 3, t.begin() + sc_pos);
            auto expr_bytecode = evaluate_expression(infix_expr);

            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());
            bytecode.push_back(Instruction(STORE, sv(var_name)));

            return sc_pos + 1;
        }
        else if (curr.value == "str") {
            if (pos + 3 >= t.size()) {
                throw std::runtime_error("Unexpected end of file after 'str' declaration at token index " + std::to_string(pos));
            }

            if (t[pos + 1].type != TOK_IDENTIFIER) {
                throw std::runtime_error("Expected variable name after 'str', but found token type " + std::to_string(t[pos + 1].type) + " ('" + t[pos + 1].value + "')");
            }
            std::string var_name = t[pos + 1].value;

            if (t[pos + 2].type != TOK_SEQ) {
                throw std::runtime_error("Expected '=' in variable declaration for '" + var_name + "', but found '" + t[pos + 2].value + "'");
            }

            size_t sc_pos = pos + 3;
            while (sc_pos < t.size() && t[sc_pos].type != TOK_SC) {
                sc_pos++;
            }
            if (sc_pos >= t.size()) {
                throw std::runtime_error("Missing closing semicolon ';' for variable declaration of '" + var_name + "'");
            }

            std::vector<Token> infix_expr(t.begin() + pos + 3, t.begin() + sc_pos);
            auto expr_bytecode = evaluate_expression(infix_expr);

            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());
            bytecode.push_back(Instruction(STORE, sv(var_name)));

            return sc_pos + 1;
        }
        else if (curr.value == "if") {
            size_t thenpos = pos;
            while (thenpos < t.size() && t[thenpos].value != "then") {
                thenpos++;
            }

            if (thenpos >= t.size()) {
                throw std::runtime_error("Missing 'then' keyword after if statement");
            }

            std::vector<Token> infix_expr(t.begin() + pos + 1, t.begin() + thenpos);
            auto expr_bytecode = evaluate_expression(infix_expr);
            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());

            size_t jump_idx = bytecode.size();
            bytecode.push_back(Instruction(JZ, sv(0.0))); // dummy jump if zero

            size_t block_pos = thenpos + 1;
            while (block_pos < t.size() && t[block_pos].value != "end") {
                block_pos = compile_block(t, block_pos, bytecode);
            }

            if (block_pos >= t.size()) {
                throw std::runtime_error("Missing 'end' keyword for if statement");
            }

            bytecode[jump_idx].operand = sv((double)bytecode.size());

            return block_pos + 1; // get past 'end'
        }
        else if (curr.value == "tbl") {
            if (t[pos + 1].type != TokenType::TOK_IDENTIFIER) {
                throw std::runtime_error("Unexpected identifier: '" + curr.value + "' at token index " + std::to_string(pos));
            }

            std::string table_name = t[pos + 1].value;

            if (t[pos + 2].type != TokenType::TOK_SEQ) {
                throw std::runtime_error("Expected '=' in table declaration for '" + table_name + "', but found '" + t[pos + 2].value + "'");
            }

            size_t lb_pos = pos + 3;
            if (t[lb_pos].type != TokenType::TOK_LB) {
                throw std::runtime_error("Expected '{' after '=' in table declaration for '" + table_name + "', but found '" + t[lb_pos].value + "'");
            }

            bytecode.push_back(Instruction(CT, sv(table_name)));

            size_t search_pos = lb_pos + 1;
            while (t[search_pos].type != TokenType::TOK_RB) {

                if (t[search_pos].type == TokenType::TOK_RB) {
                    break;
                }

                if (t[search_pos].type != TokenType::TOK_IDENTIFIER) {
                    throw std::runtime_error("Expected table key identifier, found '" + t[search_pos].value + "'");
                }

                std::string keyname = t[search_pos].value;

                if (t[search_pos + 1].type != TokenType::TOK_SEQ) {
                    throw std::runtime_error("Expected '=' after key '" + keyname + "' in table '" + table_name + "'");
                }

                size_t sc_pos = search_pos + 2;
                while (t[sc_pos].type != TokenType::TOK_SC) {
                    if (t[sc_pos].type == TokenType::TOK_RB) {
                        throw std::runtime_error("Expected ';' after value for key '" + keyname + "'");
                    }
                    sc_pos++;
                }

                bytecode.push_back(Instruction(PUSH, sv(table_name)));

                std::vector<Token> infix_expr(t.begin() + search_pos + 2, t.begin() + sc_pos);
                auto expr_bytecode = evaluate_expression(infix_expr);
                bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());

                bytecode.push_back(Instruction(STV, sv(keyname)));

                search_pos = sc_pos + 1;
            }

            return search_pos + 1;
        }
        else if (curr.value == "while") {
            size_t do_pos = pos;
            while (t[do_pos].value != "do") {
                do_pos++;
            }

            if (do_pos > t.size()) {
                throw std::runtime_error("Expected 'do' after 'while'");
            }

            std::vector<Token> infix_expr(t.begin() + pos + 1, t.begin() + do_pos);
            auto expr_bytecode = evaluate_expression(infix_expr);

            size_t expression_bytecode_index = bytecode.size(); // grab index of expression evaluation bytecode
            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end()); // bytecode for while ..... do block evaluation

            size_t jump_idx = bytecode.size(); // index of jump if zero instruction
            bytecode.push_back(Instruction(JZ, 0)); // add jump if zero 

            size_t block_pos = do_pos + 1; // start code block at token right after the 'do'
            while (block_pos < t.size() && t[block_pos].value != "end") { // stop at 'end'
                block_pos = compile_block(t, block_pos, bytecode); // compile everything inside of it
            }

            // after the block inside the while statement runs, jump back and re-evaluate the condition to check if we need to run it again

            bytecode.push_back(Instruction(JMP, expression_bytecode_index)); // jmps back to the evaluate + jz 

            bytecode[jump_idx].operand = sv((double)bytecode.size()); // patch the jz with the position in bytecode after the loop

            return block_pos + 1;

        }
        else if (curr.value == "for") {
            // initializer is at pos+1, firstcomma
            // condition is at firstcomma + 1, secondcomma
            // iterator is at secondcomma + 1, do_pos

            size_t firstcomma = pos;
            while (t[firstcomma].type != TOK_COMMA) {   // this ends the var initializer statement
                firstcomma++;
            }




            std::string varname = t[pos + 1].value;
            if (t[pos + 2].type != TokenType::TOK_SEQ) {
                throw std::runtime_error("Expected '=' after iterator variable declaration '" + varname + "' in for loop.");
            }

            // pos + 1 -> x
            // pos + 2 -> =
            // pos + 3 -> 5

           
            // get value by evaluating t[pos+3] to firstcomma


            std::vector<Token> infix_expr_initializer(t.begin() + pos + 3, t.begin() + firstcomma);
            auto initializer_expr_bytecode = evaluate_expression(infix_expr_initializer);
            bytecode.insert(bytecode.end(), initializer_expr_bytecode.begin(), initializer_expr_bytecode.end());

            // this pushes the value of the initializer expression to the stack

            bytecode.push_back(Instruction(STORE, varname)); // store that value in the initializer variable name (x)
            
            // IMPORTANT -> this will be a scope-only variable soon (not accessible in scopes above it) but it'll be global for now









            size_t secondcomma = firstcomma + 1;
            while (t[secondcomma].type != TOK_COMMA) {   // this ends the condition statement
                secondcomma++;
            }


            size_t condition_bytecode_pos = bytecode.size();

            // evaluate the condition statement


            std::vector<Token> condition_infix_expr(t.begin() + firstcomma + 1, t.begin() + secondcomma); // condition statement is between first comma and second comma
            auto condition_expr_bytecode = evaluate_expression(condition_infix_expr);
            bytecode.insert(bytecode.end(), condition_expr_bytecode.begin(), condition_expr_bytecode.end());
            // will push value of condition to the top of the stack



            size_t jze_pos = bytecode.size();
            bytecode.push_back(Instruction(JZ, 0.0)); // we dont know where the end of the bytecode is yet



            size_t do_pos = secondcomma + 1;
            while (t[do_pos].value != "do") {           // this ends the iterator statement
                do_pos++;
            }



            // find the end of the block
            size_t block_pos = do_pos + 1; // start code block at token right after the 'do'
            while (block_pos < t.size() && t[block_pos].value != "end") { // stop at 'end'
                block_pos = compile_block(t, block_pos, bytecode); // compile everything inside of it
            }

            size_t endpos = block_pos;



            // slap the iterator statement right after the code runs


            std::vector<Token> iterator_infix_expr(t.begin() + secondcomma + 3, t.begin() + do_pos); 
            auto iterator_expr_bytecode = evaluate_expression(iterator_infix_expr);
            bytecode.insert(bytecode.end(), iterator_expr_bytecode.begin(), iterator_expr_bytecode.end());
            bytecode.push_back(Instruction(STORE, varname));

            bytecode.push_back(Instruction(JMP, condition_bytecode_pos));


            bytecode[jze_pos].operand = sv((double)bytecode.size()); // patch jze so it knows to jump all the way after all the code if the condition evaluates to false

            return endpos + 1;


        } 
        else if (curr.value == "function") {  // func definition
            auto namepos = pos + 1;
            std::string funcname = t[namepos].value;

            auto lp_pos = namepos + 1;
            if (t[lp_pos].type != TokenType::TOK_LP) {
                throw std::runtime_error("Expected '(' after function declaration '" + funcname + "', but got '" + t[lp_pos].value + "'");
            }

            auto rp_pos = lp_pos + 1;
            while (rp_pos < t.size() && t[rp_pos].type != TokenType::TOK_RP) {
                rp_pos++;
            }

            std::vector<std::string> param_names;
            for (size_t i = lp_pos + 1; i < rp_pos; ++i) {
                if (t[i].type == TokenType::TOK_IDENTIFIER) {  // parse each argument name
                    param_names.push_back(t[i].value);
                }
            }

            size_t skip_jump_idx = bytecode.size();
            bytecode.push_back(Instruction(JMP, 0.0)); // for jumping over code block so func code doesnt run without calling

            bytecode.push_back(Instruction(FUNC, sv(funcname)));  // mark in bytecode where the func starts and map it to its name



            for (auto it = param_names.rbegin(); it != param_names.rend(); ++it) {
                bytecode.push_back(Instruction(STORE, sv(*it))); // pop arguments off the stack and store them (in reverse order so the caller reads them in the right order)
            }


            size_t block_pos = rp_pos + 1;
            while (block_pos < t.size() && t[block_pos].value != "end") {
                block_pos = compile_block(t, block_pos, bytecode);           // compile function contents
            }

            if (block_pos >= t.size() || t[block_pos].value != "end") {
                throw std::runtime_error("Missing 'end' keyword for function '" + funcname + "'");
            }

            bytecode.push_back(Instruction(PUSH, 0));  // default return in case user doesnt put one
            bytecode.push_back(Instruction(RET));

            bytecode[skip_jump_idx].operand = sv((double)bytecode.size()); // fill in jump with the actual addr after the code block

            return block_pos + 1;
        }
        else if (pos + 1 < t.size() && t[pos].type == TokenType::TOK_IDENTIFIER && t[pos + 1].type == TokenType::TOK_LP) {
            std::string funcname = t[pos].value;

            size_t lp_pos = pos + 1;
            size_t rp_pos = lp_pos;
            while (rp_pos < t.size() && t[rp_pos].type != TokenType::TOK_RP) {
                rp_pos++;
            }
            if (rp_pos >= t.size()) {
                throw std::runtime_error("Missing closing parenthesis for call to '" + funcname + "'");
            }

            size_t arg_start = lp_pos + 1;
            int arg_count = 0;

            if (arg_start < rp_pos) {
                size_t current_comma = arg_start;
                while (current_comma <= rp_pos) {
                    size_t next_comma = current_comma;
                    while (next_comma < rp_pos && t[next_comma].type != TOK_COMMA) {
                        next_comma++;
                    }

                    std::vector<Token> arg_expr(t.begin() + current_comma, t.begin() + next_comma);
                    auto arg_bytecode = evaluate_expression(arg_expr);
                    bytecode.insert(bytecode.end(), arg_bytecode.begin(), arg_bytecode.end());     // write bytecode to evaluate each arg, then they get pushed to stack

                    arg_count++;
                    current_comma = next_comma + 1;
                }
            }

            bytecode.push_back(Instruction(CALL, sv(funcname))); // call function (it will read the args off the stack)

            size_t sc_pos = rp_pos + 1;
            if (sc_pos < t.size() && t[sc_pos].type == TOK_SC) {
                return sc_pos + 1;
            }
            return rp_pos + 1; 
        }
        else if (t[pos].type == TokenType::TOK_IDENTIFIER && t[pos + 1].type == TokenType::TOK_DOT) {
            std::string table_name = t[pos].value;

            if (t[pos + 2].type != TokenType::TOK_IDENTIFIER) {
                throw std::runtime_error("Expected identifier after '.' for table assignment, but found '" + t[pos + 2].value + "'");
            }
            std::string key_name = t[pos + 2].value;

            if (t[pos + 3].type != TokenType::TOK_SEQ) { // '='
                throw std::runtime_error("Expected '=' in table assignment for '" + table_name + "." + key_name + "'");
            }

            size_t sc_pos = pos + 4;
            while (t[sc_pos].type != TokenType::TOK_SC) {
                if (sc_pos >= t.size()) {
                    throw std::runtime_error("Expected ';' at the end of table assignment");
                }
                sc_pos++;
            }

            bytecode.push_back(Instruction(PUSH, sv(table_name)));

            std::vector<Token> infix_expr(t.begin() + pos + 4, t.begin() + sc_pos);
            auto expr_bytecode = evaluate_expression(infix_expr);
            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end());

            bytecode.push_back(Instruction(STV, sv(key_name)));

            return sc_pos + 1;
        }
        else if (curr.type == TokenType::TOK_IDENTIFIER && t[pos + 1].type == TokenType::TOK_SEQ) {
            // setting a variable's value

            std::string var_name = curr.value;


            // find end of expression (semicolon)
            size_t sc_pos = pos;
            while (t[sc_pos].type != TokenType::TOK_SC) {
                sc_pos++;
            }


            // grab the whole value expression
            std::vector<Token> infix_expr(t.begin() + pos + 2, t.begin() + sc_pos); 
            auto expr_bytecode = evaluate_expression(infix_expr);
            bytecode.insert(bytecode.end(), expr_bytecode.begin(), expr_bytecode.end()); 
            // will evaluate then push to stack


            bytecode.push_back(Instruction(STORE, var_name)); // now store it in the var 


            return sc_pos + 1;
        }
        else if (curr.value == "return") {
            Token next = t[pos + 1];

            if (next.type == TokenType::TOK_SC) {
                bytecode.push_back(Instruction(PUSH, 0)); // default return
                bytecode.push_back(Instruction(RET));
                return pos + 2;
            }
            else {

                // find semicolon;
                size_t sc_pos = pos + 1;
                while (t[sc_pos].type != TokenType::TOK_SC) {
                    sc_pos++;
                }
                

                std::vector<Token> ret_expr(t.begin() + pos + 1, t.begin() + sc_pos);
                auto ret_val_bytecode = evaluate_expression(ret_expr);
                bytecode.insert(bytecode.end(), ret_val_bytecode.begin(), ret_val_bytecode.end());     // evaluate whatever we're returning, then it gets pushed to stack


                bytecode.push_back(Instruction(RET));

                return sc_pos + 1;

            }

        }
        else {
            throw std::runtime_error("Unknown identifier: '" + curr.value + "' at token index " + std::to_string(pos));
        }
    }

    inline std::vector<Instruction> compile(std::vector<Token> t) {
        std::vector<Instruction> bytecode;
        size_t pos = 0;

        while (pos < t.size()) {
            pos = compile_block(t, pos, bytecode);
        }

        return bytecode;
    }
}