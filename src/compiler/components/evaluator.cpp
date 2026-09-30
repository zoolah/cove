#include "../compiler.hpp"
#include <unordered_map>

std::vector<Instruction> Compiler::evaluate_expression(const std::vector<Token>& infix_expr) {
    struct OpInfo { int prec; Opcode vm_op; };
    static const std::unordered_map<TokenType, OpInfo> ops = {
        { TOK_MUL,{ 3, MUL } },{ TOK_DIV,{ 3, DIV } },{ TOK_MOD,{ 3, MOD } },
        { TOK_ADD,{ 2, ADD } },{ TOK_SUB,{ 2, SUB } },{ TOK_CONCAT,{ 2, CONCAT } },
        { TOK_EQ,{ 1, EQ } },{ TOK_NOTEQ,{ 1, NEQ } },{ TOK_LT,{ 1, LT } },{ TOK_GT,{ 1, GT } },
        { TOK_AND,{ 0, MUL } },{ TOK_OR,{ -1, ADD } },
    };

    std::vector<Token> rpn, op_stack;
    std::unordered_map<size_t, std::vector<std::vector<Token>>> call_args;
    const size_t n = infix_expr.size();

    auto is = [&](size_t k, TokenType t) { return k < n && infix_expr[k].type == t; };
    auto at = [&](size_t k) { return infix_expr.begin() + static_cast<std::ptrdiff_t>(k); };
    auto push_id = [&](const std::string& s) { rpn.emplace_back(TOK_IDENTIFIER, s); };
    auto pop_op = [&] { rpn.push_back(op_stack.back()); op_stack.pop_back(); };
    auto push_call = [&](const std::string& name, std::vector<std::vector<Token>> args) {
        call_args[rpn.size()] = std::move(args);
        push_id(name);
        };

    auto find_close = [&](size_t lp) {
        size_t rp = lp + 1;
        int depth = 1;
        for (; rp < n; ++rp) {
            if (infix_expr[rp].type == TOK_LP) depth++;
            else if (infix_expr[rp].type == TOK_RP && --depth == 0) break;
        }
        return rp;
        };

    auto parse_arguments = [&](size_t lp, size_t rp) {
        std::vector<std::vector<Token>> args;
        for (size_t cur = lp + 1; cur < rp;) {
            size_t next = cur;
            for (int nested = 0; next < rp; ++next) {
                const TokenType t = infix_expr[next].type;
                if (t == TOK_LP) nested++;
                else if (t == TOK_RP) nested--;
                else if (t == TOK_COMMA && nested == 0) break;
            }
            if (cur < next) args.emplace_back(at(cur), at(next));
            cur = next + 1;
        }
        return args;
        };

    for (size_t i = 0; i < n; ++i) {
        const Token& tok = infix_expr[i];
        const bool ident = tok.type == TOK_IDENTIFIER;
        const bool dotted_call = ident && is(i + 1, TOK_DOT) && is(i + 2, TOK_IDENTIFIER) && is(i + 3, TOK_LP);
        const bool std_call = ident && is(i + 1, TOK_LP);

        if (tok.value == "input" && is(i + 1, TOK_LP)) { 
            size_t rp = find_close(i + 1);
            auto args = parse_arguments(i + 1, rp);
            if (args.size() != 1) throw std::runtime_error("Input expects 1 argument");
            push_call("INPUT", std::move(args));
            i = rp;
        }
        else if (dotted_call || std_call) {
            const std::string name = dotted_call ? tok.value + "." + infix_expr[i + 2].value : tok.value;
            const size_t lp = dotted_call ? i + 3 : i + 1;
            const size_t rp = find_close(lp);
            if (rp >= n) throw std::runtime_error("Missing closing parenthesis in function call to '" + name + "'");
            push_call("CALL:" + name, parse_arguments(lp, rp));
            i = rp;
        }
        else if (ident && (is(i + 1, TOK_DCOLON) || is(i + 1, TOK_DOT)) && is(i + 2, TOK_IDENTIFIER)) {
            push_id(tok.value + (is(i + 1, TOK_DCOLON) ? "::" : ".") + infix_expr[i + 2].value);
            i += 2;
        }
        else if (ident && (is(i + 1, TOK_INC) || is(i + 1, TOK_DEC))) {
            rpn.push_back(tok);
            push_id(is(i + 1, TOK_INC) ? "POSTINC" : "POSTDEC");
            i += 1;
        }
        else if ((tok.type == TOK_INC || tok.type == TOK_DEC) && is(i + 1, TOK_IDENTIFIER)) {
            rpn.push_back(infix_expr[i + 1]);
            push_id(tok.type == TOK_INC ? "PREINC" : "PREDEC");
            i += 1;
        }
        else if (tok.type == TOK_NUMBER || ident || tok.type == TOK_STR) {
            rpn.push_back(tok);
        }
        else if (ops.count(tok.type)) {
            while (!op_stack.empty() && op_stack.back().type != TOK_LP &&
                ops.at(op_stack.back().type).prec >= ops.at(tok.type).prec)
                pop_op();
            op_stack.push_back(tok);
        }
        else if (tok.type == TOK_LP) {
            op_stack.push_back(tok);
        }
        else if (tok.type == TOK_RP) {
            while (!op_stack.empty() && op_stack.back().type != TOK_LP) pop_op();
            if (!op_stack.empty()) op_stack.pop_back();
        }
    }
    while (!op_stack.empty()) pop_op();



    std::vector<Instruction> bytecode;
    auto append = [&](const std::vector<Instruction>& v) { bytecode.insert(bytecode.end(), v.begin(), v.end()); };

    for (size_t idx = 0; idx < rpn.size(); ++idx) {
        const Token& tok = rpn[idx];
        const std::string& v = tok.value;

        if (tok.type == TOK_NUMBER) {
            bytecode.emplace_back(PUSH, Value(std::stod(v)));
        }
        else if (tok.type == TOK_STR) {
            bytecode.emplace_back(PUSH, Value(v));
        }
        else if (tok.type == TOK_IDENTIFIER) {
            if (v.rfind("CALL:", 0) == 0) {
                const std::string fn = v.substr(5);
                auto& args = call_args[idx];
                for (auto it = args.rbegin(); it != args.rend(); ++it) append(evaluate_expression(*it));

                if (size_t dot = fn.find('.'); dot != std::string::npos) {
                    bytecode.emplace_back(PUSH, Value(fn.substr(0, dot)));
                    bytecode.emplace_back(LTV, Value(fn.substr(dot + 1)));
                    bytecode.emplace_back(CALL);
                }
                else {
                    bytecode.emplace_back(CALL, Value(fn));
                }
            }
            else if (v == "INPUT") {
                append(evaluate_expression(call_args[idx][0]));
                bytecode.emplace_back(INP);
            }
            else if (v == "PREINC" || v == "POSTINC" || v == "PREDEC" || v == "POSTDEC") {
                if (idx == 0) throw std::runtime_error("++/-- used without an identifier");

                const std::string& target = rpn[idx - 1].value;
                const bool is_prefix = v.find("PRE") != std::string::npos;
                const Opcode step = (v.find("INC") != std::string::npos) ? ADD : SUB;
                const size_t dot = target.find('.');

                if (is_prefix) {
                    bytecode.emplace_back(PUSH, Value(1.0));
                    bytecode.emplace_back(step);
                    bytecode.emplace_back(DUP);
                }
                else {
                    bytecode.emplace_back(DUP);
                    bytecode.emplace_back(PUSH, Value(1.0));
                    bytecode.emplace_back(step);
                }

                if (dot == std::string::npos) {
                    bytecode.emplace_back(STORE, Value(target));
                }
                else {
                    bytecode.emplace_back(PUSH, Value(target.substr(0, dot)));
                    if (is_prefix) bytecode.emplace_back(SWAP);
                    bytecode.emplace_back(STV, Value(target.substr(dot + 1)));
                }
            }
            else if (size_t sep = v.find("::"); sep != std::string::npos) {
                bytecode.emplace_back(PUSH, Value(v.substr(0, sep)));
                bytecode.emplace_back(CLOAD, Value(v.substr(sep + 2)));
            }
            else if (size_t dot = v.find('.'); dot != std::string::npos) {
                bytecode.emplace_back(PUSH, Value(v.substr(0, dot)));
                bytecode.emplace_back(LTV, Value(v.substr(dot + 1)));
            }
            else {
                bytecode.emplace_back(LOAD, Value(v));
            }
        }
        else {
            auto op = ops.find(tok.type);
            if (op == ops.end()) throw std::runtime_error("Unsupported operator");
            bytecode.emplace_back(op->second.vm_op);
        }
    }

    return bytecode;
}