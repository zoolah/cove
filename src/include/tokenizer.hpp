#include <string>
#include <vector>
#include <cctype>
#include "structs.hpp"
#define isspace std::isspace
#define isletter(c) (std::isalpha(c) || (c) == '_')

namespace Tokenizer {
    inline std::vector<Token> tokenize(const std::string& source) {
        std::vector<Token> results;
        size_t pos = 0;

        while (pos < source.length()) {
            char c = source[pos];

            if (isspace(c)) {
                pos++; continue;
            }

            if (c == '(') {
                results.push_back(Token(TOK_LP));
                pos++;
                continue;
            }
            if (c == ')') {
                results.push_back(Token(TOK_RP));
                pos++;
                continue;
            }

            if (isletter(c)) {
                std::string buf;
                while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_')) {
                    buf += source[pos];
                    pos++;
                }

                if (buf == "and") {
                    results.push_back(Token(TOK_AND, buf));
                }
                else if (buf == "or") {
                    results.push_back(Token(TOK_OR, buf));
                }
                else {
                    results.push_back(Token(TOK_IDENTIFIER, buf));
                }
                
                
                continue;
            }

            if (c == '"') {
                pos++;
                std::string buf;
                while (pos < source.length() && source[pos] != '"') {
                    buf += source[pos];
                    pos++;
                }
                pos++;
                results.push_back(Token(TOK_STR, buf));
                continue;
            }

            if (c == '\'') {
                pos++;
                std::string buf;
                while (pos < source.length() && source[pos] != '\'') {
                    buf += source[pos];
                    pos++;
                }
                pos++;
                results.push_back(Token(TOK_STR, buf));
                continue;
            }

            if (c == '+') { results.push_back(Token(TOK_ADD)); pos++; continue; }
            if (c == '-') { results.push_back(Token(TOK_SUB)); pos++; continue; }
            if (c == '/') { results.push_back(Token(TOK_DIV)); pos++; continue; }
            if (c == '%') { results.push_back(Token(TOK_MOD)); pos++; continue; }
            if (c == '*') { results.push_back(Token(TOK_MUL)); pos++; continue; }
            if (c == '&') { results.push_back(Token(TOK_AND)); pos++; continue; }
            if (c == '|') { results.push_back(Token(TOK_OR)); pos++; continue; }
            if (c == '{') { results.push_back(Token(TOK_LB)); pos++; continue; }
            if (c == '}') { results.push_back(Token(TOK_RB)); pos++; continue; }
            if (c == '.' && source[pos + 1] == '.') {
                results.push_back(Token(TOK_CONCAT));
                pos += 2;
                continue;
            }
            if (c == '.') {
                results.push_back(Token(TOK_DOT));
                pos++;
                continue;
            }

            if (c == ',') {
                results.push_back(Token(TOK_COMMA));
                pos++;
                continue;
            }

            if (c == '~' && source[pos + 1] == '=') {
                results.push_back(Token(TOK_NOTEQ));
                pos += 2;
                continue;
            }
            if (c == '=' && source[pos + 1] == '=') {
                results.push_back(Token(TOK_EQ));
                pos += 2;
                continue;
            }
            if (c == '<') { results.push_back(Token(TOK_LT)); pos++; continue; }
            if (c == '>') { results.push_back(Token(TOK_GT)); pos++; continue; }

            if (c == '=') { results.push_back(Token(TOK_SEQ)); pos++; continue; }
            if (c == ';') { results.push_back(Token(TOK_SC)); pos++; continue; }

            if (std::isdigit(c)) {
                std::string buf;
                bool found_decimal = false;
                while (pos < source.length()) {
                    if (std::isdigit(source[pos])) {
                        buf += source[pos];
                    }
                    else if (source[pos] == '.') {
                        if (found_decimal) throw std::runtime_error("Malformed number");
                        buf += source[pos];
                        found_decimal = true;
                    }
                    else {
                        break;
                    }
                    pos++;
                }
                results.push_back(Token(TOK_NUMBER, buf));
                continue;
            }

            pos++;
        }

        return results;
    }
}