#include "../compiler.hpp"

namespace Compiler {

static size_t end_of_call(const Tokens& t, size_t rp) {
    size_t sc = rp + 1;
    return (sc < t.size() && t[sc].type == TOK_SC) ? sc + 1 : rp + 1;
}

size_t compile_print(const Tokens& t, size_t pos, Bytecode& bytecode) {
    expect(pos + 3 < t.size(), "Unexpected end of file after 'print'");
    expect(t[pos + 1].type == TOK_LP, "Invalid print statement syntax");

    size_t rp = find_close(t, pos + 1);
    expect(rp < t.size(), "Missing closing parenthesis for print");

    compile_expression(t, pos + 2, rp, bytecode);
    bytecode.push_back(Instruction(PRINT));

    expect(rp + 1 < t.size() && t[rp + 1].type == TOK_SC, "Missing closing semicolon ';' after print statement");
    return rp + 2;
}

size_t compile_decl(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& kw = t[pos].value;
    expect(pos + 3 < t.size(), "Unexpected end of file after '" + kw + "' declaration at token index " + std::to_string(pos));
    expect(t[pos + 1].type == TOK_IDENTIFIER,
        "Expected variable name after '" + kw + "', but found token type " + std::to_string(t[pos + 1].type) + " ('" + t[pos + 1].value + "')");

    const std::string var = t[pos + 1].value;
    expect(t[pos + 2].type == TOK_SEQ,
        "Expected '=' in variable declaration for '" + var + "', but found '" + t[pos + 2].value + "'");

    size_t sc = find_type(t, pos + 3, TOK_SC);
    expect(sc < t.size(), "Missing closing semicolon ';' for variable declaration of '" + var + "'");

    compile_expression(t, pos + 3, sc, bytecode);
    bytecode.push_back(Instruction(STORE, Value(var)));
    return sc + 1;
}

size_t compile_assign(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& var = t[pos].value;

    size_t sc = find_type(t, pos, TOK_SC);
    expect(sc < t.size(), "Missing closing semicolon ';' after assignment to '" + var + "'");

    bool postfix_self = sc == pos + 4 &&
        t[pos + 2].type == TOK_IDENTIFIER && t[pos + 2].value == var &&
        (t[pos + 3].type == TOK_INC || t[pos + 3].type == TOK_DEC);

    compile_expression(t, pos + 2, sc, bytecode);
    if (!postfix_self) bytecode.push_back(Instruction(STORE, Value(var)));
    return sc + 1;
}

size_t compile_incdec_stmt(const Tokens& t, size_t pos, Bytecode& bytecode) {
    size_t sc = find_type(t, pos + 1, TOK_SC);
    expect(sc < t.size(), "Missing closing semicolon ';' after increment/decrement statement");

    compile_expression(t, pos, sc, bytecode);
    bytecode.push_back(Instruction(POP));
    return sc + 1;
}

size_t compile_return(const Tokens& t, size_t pos, Bytecode& bytecode) {
    if (t[pos + 1].type == TOK_SC) {
        bytecode.push_back(Instruction(PUSH, 0)); 
        bytecode.push_back(Instruction(RET));
        return pos + 2;
    }

    size_t sc = find_type(t, pos + 1, TOK_SC);
    expect(sc < t.size(), "Missing closing semicolon ';' after return statement");

    compile_expression(t, pos + 1, sc, bytecode);
    bytecode.push_back(Instruction(RET));
    return sc + 1;
}

size_t compile_call(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& func_name = t[pos].value;

    size_t lp = pos + 1;
    size_t rp = find_close(t, lp);
    expect(rp < t.size(), "Missing closing parenthesis for call to '" + func_name + "'");

    compile_args(t, lp, rp, bytecode);
    bytecode.push_back(Instruction(CALL, Value(func_name))); 
    return end_of_call(t, rp);
}

size_t compile_method_call(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& table = t[pos].value;
    const std::string& key = t[pos + 2].value;

    size_t lp = pos + 3;
    size_t rp = find_close(t, lp);
    expect(rp < t.size(), "Missing closing parenthesis in method call to '" + table + "." + key + "'");

    compile_args(t, lp, rp, bytecode);
    bytecode.push_back(Instruction(PUSH, Value(table)));
    bytecode.push_back(Instruction(LTV, Value(key)));
    bytecode.push_back(Instruction(CALL));
    return end_of_call(t, rp);
}

size_t compile_member_function_call(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& instance = t[pos].value;
    const std::string& function = t[pos + 2].value;
    size_t lp = pos + 3;
    size_t rp = find_close(t, lp);
    expect(rp < t.size(), "Missing closing parenthesis in member function call to '" + instance + "::" + function + "'");

    compile_args(t, lp, rp, bytecode);
    bytecode.push_back(Instruction(PUSH, Value(instance)));
    bytecode.push_back(Instruction(CMFUNC, Value(function)));
    bytecode.push_back(Instruction(POP));
    return end_of_call(t, rp);
}

size_t compile_table_assign(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& table = t[pos].value;
    expect(t[pos + 2].type == TOK_IDENTIFIER,
        "Expected identifier after '.' for table assignment, but found '" + t[pos + 2].value + "'");

    const std::string& key = t[pos + 2].value;
    expect(t[pos + 3].type == TOK_SEQ, "Expected '=' in table assignment for '" + table + "." + key + "'");

    size_t sc = find_type(t, pos + 4, TOK_SC);
    expect(sc < t.size(), "Expected ';' at the end of table assignment");

    bytecode.push_back(Instruction(PUSH, Value(table)));
    compile_expression(t, pos + 4, sc, bytecode);
    bytecode.push_back(Instruction(STV, Value(key)));
    return sc + 1;
}

size_t compile_member_assign(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string& cls = t[pos].value;
    expect(t[pos + 2].type == TOK_IDENTIFIER, "Expected class member name after '::'");

    const std::string& member = t[pos + 2].value;
    expect(t[pos + 3].type == TOK_SEQ, "Expected '=' after class member '" + cls + "::" + member + "'");

    size_t sc = find_type(t, pos + 4, TOK_SC);
    expect(sc < t.size(), "Missing closing semicolon after class member assignment");

    compile_expression(t, pos + 4, sc, bytecode);
    bytecode.push_back(Instruction(PUSH, Value(cls)));
    bytecode.push_back(Instruction(PUSH, Value(member)));
    bytecode.push_back(Instruction(CSTORE));
    return sc + 1;
}


} 