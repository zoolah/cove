#pragma once
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>
#include <iostream>

enum class ValueType { NUMBER, STRING, FUNCTION };



typedef enum {
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	POP,
	PUSH,
	PRINT,
	STORE,
	LOAD,
	NEQ,
	EQ,
	LT,
	GT,
	JZ,
	JNZ,
	JNE,
	JE,
	CONCAT,
	CT,
	STV,
	LTV,
	JMP,
	FUNC, 
	CALL, 
	RET,
	INP
} Opcode;



struct sv {
	ValueType type;
	double num = 0.0; // ints and floats both use double
	std::string str = "";

	sv(double n) : type(ValueType::NUMBER), num(n) {}
	sv(std::string s) : type(ValueType::STRING), str(s) {}
	sv(std::string s, ValueType t) : type(t), str(s) {
		if (t == ValueType::NUMBER) {
			type = ValueType::NUMBER;
			num = std::stod(s);
		}
	}
	sv() : type(ValueType::NUMBER), num(0.0) {}
};


struct Instruction {
	Opcode op;
	sv operand; 

	Instruction(Opcode o, sv v = sv((uint64_t)0)) : op(o), operand(v) {}
};


enum TokenType { 
	TOK_IDENTIFIER, // any identifier (print, var name, etc)
	TOK_NUMBER,// number literal
	TOK_STR,   // string literal
	TOK_LP,    // (
	TOK_RP,    // )
	TOK_EOF,   // end of file
	TOK_ADD,   // +
	TOK_SUB,   // -
	TOK_DIV,   // /
	TOK_MOD,   // %
	TOK_MUL,   // *
	TOK_SEQ,   // =
	TOK_SC,    // ;
	TOK_EQ,    // ==
	TOK_NOTEQ, // ~=
	TOK_LT,    // <
	TOK_GT,    // >
	TOK_AND,   // &
	TOK_OR,    // |
	TOK_CONCAT,// ..
	TOK_LB,	   // {
	TOK_RB,     // }
	TOK_DOT,   // .
	TOK_COMMA  // ,
};

struct Token {
	TokenType type;
	std::string value;

	Token(TokenType type_) { type = type_; value = ""; }
	Token(TokenType type_, std::string value_) { type = type_; value = value_; }
};


class Stack {
private:
	std::vector<sv> data_;

public:
	void push(sv value) {
		data_.push_back(std::move(value));
	}

	sv pop() {
		if (data_.empty()) {
			throw std::runtime_error("Stack underflow");
		}
		sv value = std::move(data_.back());
		data_.pop_back();
		return value;
	}
};



