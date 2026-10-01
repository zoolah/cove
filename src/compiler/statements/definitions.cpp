#include "../compiler.hpp"

namespace Compiler {

size_t compile_function(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string func_name = t[pos + 1].value;

    size_t lp = pos + 2;
    expect(t[lp].type == TOK_LP,
        "Expected '(' after function declaration '" + func_name + "', but got '" + t[lp].value + "'");

    size_t rp = find_type(t, lp + 1, TOK_RP); 
    std::vector<std::string> params;
    for (size_t i = lp + 1; i < rp; ++i) {
        if (t[i].type == TOK_IDENTIFIER) params.push_back(t[i].value);
    }

    size_t skip = bytecode.size();
    bytecode.push_back(Instruction(JMP, 0.0)); 
    bytecode.push_back(Instruction(FUNC, Value(func_name))); 

    for (auto it = params.rbegin(); it != params.rend(); ++it) {
        bytecode.push_back(Instruction(STORE, Value(*it))); 
    }

    size_t end_pos = compile_until_end(t, rp + 1, bytecode);
    expect(end_pos < t.size(), "Missing 'end' keyword for function '" + func_name + "'");

    bytecode.push_back(Instruction(PUSH, 0)); 
    bytecode.push_back(Instruction(RET));

    bytecode[skip].operand = Value((double)bytecode.size());
    return end_pos + 1;
}

size_t compile_class(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string class_name = t[pos + 1].value;

    size_t lb = pos + 2;
    expect(t[lb].type == TOK_LB, "'{' Expected after class declaration '" + class_name + "'");

    bytecode.push_back(Instruction(CDEF, Value(class_name))); 


    
    size_t cursor = lb + 1;
    int depth = 1;
    for (; cursor < t.size(); ++cursor) {
        const Token& c = t[cursor];
        if (c.type == TOK_RB) depth--;
        if (c.type == TOK_LB) depth++;

        if (c.value == "num" || c.value == "str") {
            expect(t[cursor + 1].type == TOK_IDENTIFIER, "Expected identifier after 'num' in class definition");
            const std::string& name = t[cursor + 1].value;
            expect(t[cursor + 2].type == TOK_SC, "Expected ';' after " + name);

            bytecode.push_back(Instruction(c.value == "num" ? CNUM : CSTR, Value(name)));
        
        }

        // find constructor
        if (c.value == class_name && t[cursor + 1].type == TOK_LP) { // classname(
            auto lp = cursor + 1;

            size_t rp = find_type(t, lp + 1, TOK_RP); // )
            std::vector<std::string> params;

            for (size_t i = lp + 1; i < rp; ++i) {
                if (t[i].type == TOK_IDENTIFIER) params.push_back(t[i].value); // (arg1, arg2, arg3)
            }

            size_t skip_op_pos = bytecode.size();
            bytecode.push_back(Instruction(JMP, 0.0));
            bytecode.push_back(Instruction(CONSTRUCTOR));                   // marker for constructor func
            for (auto it = params.rbegin(); it != params.rend(); ++it) {
                bytecode.push_back(Instruction(STORE, Value(*it)));         // grab the arguments from the stack and store in the scope
            }


            size_t end = compile_until_end(t, rp + 1, bytecode);            // compile function body


            expect(end < t.size(), "Missing 'end' keyword for class constructor '" + class_name + "'");


            bytecode[skip_op_pos].operand = Value((double)bytecode.size());





        }

        if (depth == 0) break;
    }
    return cursor + 1;
}


size_t compile_table_def(const Tokens& t, size_t pos, Bytecode& bytecode) {
    expect(t[pos + 1].type == TOK_IDENTIFIER,
        "Unexpected identifier: '" + t[pos].value + "' at token index " + std::to_string(pos));

    const std::string table = t[pos + 1].value;
    expect(t[pos + 2].type == TOK_SEQ,
        "Expected '=' in table declaration for '" + table + "', but found '" + t[pos + 2].value + "'");

    size_t lb = pos + 3;
    expect(t[lb].type == TOK_LB,
        "Expected '{' after '=' in table declaration for '" + table + "', but found '" + t[lb].value + "'");

    bytecode.push_back(Instruction(CT, Value(table)));

    size_t cur = lb + 1;
    while (cur < t.size() && t[cur].type != TOK_RB) {
        expect(t[cur].type == TOK_IDENTIFIER, "Expected table key identifier, found '" + t[cur].value + "'");
        const std::string key = t[cur].value;

        expect(t[cur + 1].type == TOK_SEQ, "Expected '=' after key '" + key + "' in table '" + table + "'");

        size_t sc = cur + 2;
        while (sc < t.size() && t[sc].type != TOK_SC && t[sc].type != TOK_RB) sc++;
        expect(sc < t.size() && t[sc].type == TOK_SC, "Expected ';' after value for key '" + key + "'");

        bytecode.push_back(Instruction(PUSH, Value(table)));
        compile_expression(t, cur + 2, sc, bytecode);
        bytecode.push_back(Instruction(STV, Value(key)));

        cur = sc + 1;
    }
    expect(cur < t.size(), "Missing closing '}' in table declaration for '" + table + "'");
    return cur + 1;
}

size_t compile_new(const Tokens& t, size_t pos, Bytecode& bytecode) {
    const std::string object = t[pos + 1].value;

    size_t lp = pos + 2;
    expect(t[lp].type == TOK_LP, "Expected '(' after object initialization of '" + object + "'");

    size_t rp = find_close(t, lp);
    expect(rp < t.size(), "Expected ')' after '(' in object initialization of '" + object + "'");

    size_t class_pos = rp + 1;
    expect(t[class_pos].type == TOK_IDENTIFIER, "Expected indentifier as class name for object '" + object + "'");
    const std::string class_name = t[class_pos].value;

    size_t sc = class_pos + 1;
    expect(t[sc].type == TOK_SC, "Expected ';' after class instantiation");

    bytecode.push_back(Instruction(PUSH, Value(object)));
    bytecode.push_back(Instruction(INSTC, Value(class_name)));

    // call constructor
    return sc + 1;
}

} 