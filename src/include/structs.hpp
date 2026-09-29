#pragma once
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>
#include <iostream>

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
	INP,
	DUP,
	SWAP
} Opcode;



struct FunctionReference {
	std::string name;
};

class Value {
private:
	std::variant<double, std::string, FunctionReference> data_;

public:
	Value() : data_(0.0) {}
	Value(double number) : data_(number) {}
	Value(std::string text) : data_(std::move(text)) {}

	static Value function(std::string name) {
		Value value;
		value.data_ = FunctionReference{ std::move(name) };
		return value;
	}

	bool is_number() const { return std::holds_alternative<double>(data_); }
	bool is_string() const { return std::holds_alternative<std::string>(data_); }
	bool is_function() const { return std::holds_alternative<FunctionReference>(data_); }

	double as_number() const { return std::get<double>(data_); }
	const std::string& as_string() const { return std::get<std::string>(data_); }
	const std::string& as_function() const { return std::get<FunctionReference>(data_).name; }
};


struct Instruction {
	Opcode op;
	Value operand;

	Instruction(Opcode o, Value v = Value()) : op(o), operand(std::move(v)) {}
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
	TOK_COMMA,  // ,
	TOK_INC,  // ++
	TOK_DEC   // --
};

struct Token {
	TokenType type;
	std::string value;

	Token(TokenType type_) { type = type_; value = ""; }
	Token(TokenType type_, std::string value_) { type = type_; value = value_; }
};


class Stack {
private:
	std::vector<Value> data_;

public:
	void push(Value value) {
		data_.push_back(std::move(value));
	}

	Value pop() {
		if (data_.empty()) {
			throw std::runtime_error("Stack underflow");
		}
		Value value = std::move(data_.back());
		data_.pop_back();
		return value;
	}

	size_t size() {
		return data_.size();
	}
};



