#include "../compiler.hpp"

namespace Compiler {

size_t compile_if(const Tokens& t, size_t pos, Bytecode& bytecode) {
    size_t then_pos = find_value(t, pos + 1, "then");
    expect(then_pos < t.size(), "Missing 'then' keyword after if statement");

    compile_expression(t, pos + 1, then_pos, bytecode);

    size_t jz = bytecode.size();
    bytecode.push_back(Instruction(JZ, Value(0.0))); 

    size_t end_pos = compile_until_end(t, then_pos + 1, bytecode);
    expect(end_pos < t.size(), "Missing 'end' keyword for if statement");

    bytecode[jz].operand = Value((double)bytecode.size()); 
    return end_pos + 1;
}

size_t compile_while(const Tokens& t, size_t pos, Bytecode& bytecode) {
    size_t do_pos = find_value(t, pos + 1, "do");
    expect(do_pos < t.size(), "Expected 'do' after 'while'");

    size_t cond_start = bytecode.size(); 
    compile_expression(t, pos + 1, do_pos, bytecode);

    size_t jz = bytecode.size();
    bytecode.push_back(Instruction(JZ, 0)); 

    size_t end_pos = compile_until_end(t, do_pos + 1, bytecode);

    bytecode.push_back(Instruction(JMP, cond_start)); 
    bytecode[jz].operand = Value((double)bytecode.size()); 
    return end_pos + 1;
}

size_t compile_for(const Tokens& t, size_t pos, Bytecode& bytecode) {
    size_t comma1 = find_type(t, pos + 1, TOK_COMMA);
    expect(comma1 < t.size(), "Expected ',' after for loop initializer");

    const std::string& var = t[pos + 1].value;
    expect(t[pos + 2].type == TOK_SEQ, "Expected '=' after iterator variable declaration '" + var + "' in for loop.");

    compile_expression(t, pos + 3, comma1, bytecode);
    bytecode.push_back(Instruction(STORE, Value(var)));

    size_t comma2 = find_type(t, comma1 + 1, TOK_COMMA);
    expect(comma2 < t.size(), "Expected ',' after for loop condition");

    size_t cond_start = bytecode.size();
    compile_expression(t, comma1 + 1, comma2, bytecode);

    size_t jz = bytecode.size();
    bytecode.push_back(Instruction(JZ, 0.0)); 

    size_t do_pos = find_value(t, comma2 + 1, "do");
    expect(do_pos < t.size(), "Expected 'do' in for loop");

    size_t end_pos = compile_until_end(t, do_pos + 1, bytecode);

    size_t iter = comma2 + 1;
    while (iter < do_pos && (t[iter].type == TOK_SC || t[iter].value.empty())) iter++;
    compile_expression(t, iter, do_pos, bytecode);

    bytecode.push_back(Instruction(JMP, cond_start));
    bytecode[jz].operand = Value((double)bytecode.size()); 
    return end_pos + 1;
}

} 