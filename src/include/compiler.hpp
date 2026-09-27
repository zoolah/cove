#pragma once
#include <string>
#include <vector>
#include <cctype>
#include "structs.hpp"
#define isspace std::isspace
#define isletter(c) (std::isalpha(c) || (c) == '_')


namespace Compiler {

    inline void print_bytecode(const std::vector<Instruction>& bytecode) {
        for (size_t i = 0; i < bytecode.size(); ++i) {
            const auto& instr = bytecode[i];
            std::cout << i << ": ";

            switch (instr.op) {
            case PUSH:   std::cout << "PUSH\t" << (instr.operand.type == ValueType::NUMBER ? std::to_string(instr.operand.num) : instr.operand.str); break;
            case POP:    std::cout << "POP"; break;
            case ADD:    std::cout << "ADD"; break;
            case SUB:    std::cout << "SUB"; break;
            case MUL:    std::cout << "MUL"; break;
            case DIV:    std::cout << "DIV"; break;
            case PRINT:  std::cout << "PRINT"; break;
            case STORE:  std::cout << "STORE\t" << instr.operand.str; break;
            case LOAD:   std::cout << "LOAD\t" << instr.operand.str; break;
            case EQ:     std::cout << "EQ"; break;
            case LT:     std::cout << "LT"; break;
            case GT:     std::cout << "GT"; break;
            case NEQ:    std::cout << "NEQ"; break;
            case JZ:     std::cout << "JZ\t" << instr.operand.num; break;
            case JNZ:    std::cout << "JNZ\t" << instr.operand.num; break;
            case JNE:    std::cout << "JNE\t" << instr.operand.num; break;
            case JE:     std::cout << "JE\t" << instr.operand.num; break;
            case CONCAT: std::cout << "CONCAT"; break;
            case CT:     std::cout << "CT\t" << instr.operand.str; break; 
            case STV:    std::cout << "STV\t" << instr.operand.str; break; 
            case LTV:    std::cout << "LTV\t" << instr.operand.str; break; 
            default:     std::cout << "UNKNOWN"; break;
            }
            std::cout << "\n";
        }
    }

    inline std::vector<Instruction> evaluate_expression(const std::vector<Token>& infix_expr) {
        std::vector<Instruction> bytecode;
        std::vector<Token> rpn_tokens;
        std::vector<Token> op_stack;

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
            else if (tok.type == TOK_NUMBER || tok.type == TOK_IDENTIFIER || tok.type == TOK_STR) {
                rpn_tokens.push_back(tok);
            }
            else if (tok.type == TOK_ADD || tok.type == TOK_SUB || tok.type == TOK_MUL ||
                tok.type == TOK_DIV || tok.type == TOK_MOD || tok.type == TOK_EQ ||
                tok.type == TOK_NOTEQ || tok.type == TOK_LT || tok.type == TOK_GT ||
                tok.type == TOK_AND || tok.type == TOK_OR || tok.type == TOK_CONCAT) {
                while (!op_stack.empty() && op_stack.back().type != TOK_LP && precedence(op_stack.back().type) >= precedence(tok.type)) {
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

        for (const auto& tok : rpn_tokens) {
            if (tok.type == TOK_NUMBER) {
                bytecode.push_back(Instruction(PUSH, sv(std::stod(tok.value))));
            }
            else if (tok.type == TOK_STR) {
                bytecode.push_back(Instruction(PUSH, sv(tok.value)));
            }
            else if (tok.type == TOK_IDENTIFIER) {
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

            size_t rp_pos = pos + 2;
            while (rp_pos < t.size() && t[rp_pos].type != TOK_RP) {
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