#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include "../shared/structs.hpp"
#define isspace std::isspace
#define isletter(c) (std::isalpha(c) || (c) == '_')

namespace Compiler {
    using Tokens   = std::vector<Token>;
    using Bytecode = std::vector<Instruction>;

    Tokens   tokenize(const std::string& source);
    void     print_bytecode(const Bytecode& bytecode);
    Bytecode evaluate_expression(const Tokens& infix_expr);


    size_t compile_print(const Tokens& t, size_t pos, Bytecode& bytecode);        
    size_t compile_decl(const Tokens& t, size_t pos, Bytecode& bytecode);          
    size_t compile_assign(const Tokens& t, size_t pos, Bytecode& bytecode);       
    size_t compile_incdec_stmt(const Tokens& t, size_t pos, Bytecode& bytecode);  
    size_t compile_return(const Tokens& t, size_t pos, Bytecode& bytecode);       
    size_t compile_call(const Tokens& t, size_t pos, Bytecode& bytecode);          
    size_t compile_method_call(const Tokens& t, size_t pos, Bytecode& bytecode);  
    size_t compile_table_assign(const Tokens& t, size_t pos, Bytecode& bytecode);  
    size_t compile_member_assign(const Tokens& t, size_t pos, Bytecode& bytecode); 

    size_t compile_if(const Tokens& t, size_t pos, Bytecode& bytecode);
    size_t compile_while(const Tokens& t, size_t pos, Bytecode& bytecode);
    size_t compile_for(const Tokens& t, size_t pos, Bytecode& bytecode);

    size_t compile_function(const Tokens& t, size_t pos, Bytecode& bytecode);     
    size_t compile_class(const Tokens& t, size_t pos, Bytecode& bytecode);         
    size_t compile_table_def(const Tokens& t, size_t pos, Bytecode& bytecode);     
    size_t compile_new(const Tokens& t, size_t pos, Bytecode& bytecode);          

    inline size_t compile_block(const Tokens& t, size_t pos, Bytecode& bytecode);

    inline void expect(bool ok, const std::string& msg) {
        if (!ok) throw std::runtime_error(msg);
    }

    inline size_t find_type(const Tokens& t, size_t from, TokenType type) {
        while (from < t.size() && t[from].type != type) from++;
        return from;
    }

    inline size_t find_value(const Tokens& t, size_t from, const std::string& value) {
        while (from < t.size() && t[from].value != value) from++;
        return from;
    }

    inline size_t find_close(const Tokens& t, size_t lp) {
        int depth = 1;
        for (size_t i = lp + 1; i < t.size(); ++i) {
            if (t[i].type == TOK_LP) depth++;
            else if (t[i].type == TOK_RP && --depth == 0) return i;
        }
        return t.size();
    }

    inline Tokens slice(const Tokens& t, size_t from, size_t to) {
        return Tokens(t.begin() + static_cast<std::ptrdiff_t>(from),
                      t.begin() + static_cast<std::ptrdiff_t>(to));
    }

    inline void compile_expression(const Tokens& t, size_t from, size_t to, Bytecode& bytecode) {
        Bytecode code = evaluate_expression(slice(t, from, to));
        bytecode.insert(bytecode.end(), code.begin(), code.end());
    }

    inline void compile_args(const Tokens& t, size_t lp, size_t rp, Bytecode& bytecode) {
        for (size_t cur = lp + 1; cur < rp;) {
            size_t next = cur;
            for (int nested = 0; next < rp; ++next) {
                const TokenType type = t[next].type;
                if (type == TOK_LP) nested++;
                else if (type == TOK_RP) nested--;
                else if (type == TOK_COMMA && nested == 0) break;
            }
            if (cur < next) compile_expression(t, cur, next, bytecode);
            cur = next + 1;
        }
    }

    inline size_t compile_until_end(const Tokens& t, size_t pos, Bytecode& bytecode) {
        while (pos < t.size() && t[pos].value != "end") pos = compile_block(t, pos, bytecode);
        return pos;
    }

    inline size_t compile_block(const Tokens& t, size_t pos, Bytecode& bytecode) {
        if (pos >= t.size()) return pos;

        const Token& curr = t[pos];
        if (curr.type != TOK_IDENTIFIER) {
            throw std::runtime_error("Unexpected token '" + curr.value + "' at root level (index " + std::to_string(pos) + ")");
        }

        const std::string& kw = curr.value;
        auto next_is = [&](size_t off, TokenType type) { return pos + off < t.size() && t[pos + off].type == type; };

        const bool is_member_assign = pos + 3 < t.size() && next_is(1, TOK_DCOLON);                         
        const bool is_call          = next_is(1, TOK_LP);                                                  
        const bool is_method_call   = next_is(1, TOK_DOT) && next_is(2, TOK_IDENTIFIER) && next_is(3, TOK_LP); 
        const bool is_table_assign  = next_is(1, TOK_DOT);                                                  
        const bool is_incdec        = next_is(1, TOK_INC) || next_is(1, TOK_DEC);                          
        const bool is_assign        = next_is(1, TOK_SEQ);                                                

        if (kw == "print")               return compile_print(t, pos, bytecode);
        if (kw == "num" || kw == "str")  return compile_decl(t, pos, bytecode);
        if (kw == "if")                  return compile_if(t, pos, bytecode);
        if (kw == "tbl")                 return compile_table_def(t, pos, bytecode);
        if (kw == "while")               return compile_while(t, pos, bytecode);
        if (kw == "for")                 return compile_for(t, pos, bytecode);
        if (kw == "function")            return compile_function(t, pos, bytecode);
        if (kw == "class")               return compile_class(t, pos, bytecode);
        if (kw == "new")                 return compile_new(t, pos, bytecode);

        if (is_member_assign)            return compile_member_assign(t, pos, bytecode);
        if (kw == "input")               throw std::runtime_error("input() can only be used in an expression");
        if (is_call)                     return compile_call(t, pos, bytecode);
        if (is_method_call)              return compile_method_call(t, pos, bytecode);
        if (is_table_assign)             return compile_table_assign(t, pos, bytecode);
        if (is_incdec)                   return compile_incdec_stmt(t, pos, bytecode);
        if (is_assign)                   return compile_assign(t, pos, bytecode);
        if (kw == "return")              return compile_return(t, pos, bytecode);

        throw std::runtime_error("Unknown identifier: '" + curr.value + "' at token index " + std::to_string(pos));
    }

    inline Bytecode compile(std::string source_code) {
        Tokens t = tokenize(source_code);
        Bytecode bytecode;
        for (size_t pos = 0; pos < t.size();) pos = compile_block(t, pos, bytecode);
        return bytecode;
    }
}