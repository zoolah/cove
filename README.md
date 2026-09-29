# Cove

Cove is a lightweight, custom stack-based programming language implemented in C++ with Lua-like syntax

WASM Powered Cove Playground (write & execute cove code) - [https://zoolah.github.io/cove/web](https://zoolah.github.io/cove/web/index.html)
Documentation Site - https://zoolah.github.io/cove/web/documentation.html

## What it supports

- `num` and `str` values
- arithmetic: `+ - * / %`
- comparisons: `== ~= < >`
- conditionals: `if ... then ... end`
- while loops: `while expression do ... end`
- for loops: `for i = 0, i < 10, i = i + 1 do ... end`
- logical chaining: `and`, `or`
- string concatenation: `..`
- tables with dot access: `profile.name`
- tables can hold function values and call them through the table: `obj.fn(...)`
- table scoping matches variable scoping, including nested function calls: inner tables shadow outer ones and leave scope when the call returns
- variable reassignment after declaration: `varname = anyexpression;`
- function declarations: `function name(arg1, arg2) ... end`
- function calls: `name(value1, value2)` and return values with `return expression`
- nested argument expressions and nested parentheses inside function calls
- string input with `input(prompt)`

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

To build in Release mode:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable will be in `build/Release/cove.exe`

## Quick start

```bash
cove.exe program.txt
```

The source will be tokenized, compiled, and executed in the VM.



# Documentation

## Variables & Arithmetics

```
num x = 5;
num y = 10;

num z = x + y;

print(z / 5);

str name = "Adam";
print(name);
```

Numerical variables are declared with `num x = value`, string variables are declared with `str x = value`. 

## If statements 

```cove
num score = 42;

if score > 10 then
    print("high score");
end
```

## While loops

A `while` loop evaluates its condition before every iteration and stops when the condition becomes false. 

```cove
num count = 0;

while count < 3 do
    print(count);
    count = count + 1;
end
```

## For loops

Cove supports a C-style loop header with initializer, condition, and iterator update separated by commas:

```cove
for i = 0, i < 5, i = i + 1 do
    print("iteration: " .. i);
end
```

The loop initializes `i`, evaluates the condition before each pass, and runs the iterator expression after each iteration.

## Functions

Function bodies are written between `function ... end`, and a function is called by writing its name followed by a parenthesized argument list.

```cove
function greet(name)
    print("hello, " .. name);
end

greet("Cove");
greet("Ada");

function sum(a, b)
    return a + b;
end

print(sum(10, 5));
print(sum(4 * 3, 9 / 3));
```

Function calls can include nested expressions inside the argument list, and arguments are evaluated before the function body runs. A function can return an expression with `return`, but if no such statement is written, it will return  `0`.

## Input

`input(prompt)` prints its string prompt, reads one token from standard input, and returns it as a string. Cove does not implicitly convert input strings to numbers, so using an input result in arithmetic raises a runtime error.

```cove
str name = input("What is your name? ");
print("Hello, " .. name);
```

## Tables

Tables are scoped the same way as variables. When a function is called, Cove creates a fresh table scope for that call. Table names are searched from the innermost scope outward, so a nested table can be chosen before an outer one without overwriting it. Tables created in a function are no longer visible once that function returns.

Tables can also store function references and call them by table lookup.

```cove
tbl profile = {
  name = "Ada";
  role = "engineer";
  level = 7;
}

print(profile.name);
profile.role = "architect";
print(profile.role);

function make_local_profile()
    tbl profile = {
        name = "Grace";
        role = "inventor";
    }

    print(profile.name);
end

make_local_profile();
print(profile.name);

function greet(name)
    print("hello " .. name);
end

tbl obj = {
    fn = greet;
}

obj.fn("Ada");
```

In this example, the table created inside `make_local_profile` is local to that call and does not replace the outer `profile` after the function returns. The `obj.fn(...)` call resolves the stored `fn` field to a function value, then calls it with the provided arguments.

## VM opcode reference

The tokenizer & compiler turn readable code into a linear set made up of the following instructions:

```text
ADD      Pop a, pop b, push b + a
SUB      Pop a, pop b, push b - a
MUL      Pop a, pop b, push b * a
DIV      Pop a, pop b, push b / a
MOD      Pop a, pop b, push b % a
POP      Discard the top stack value
PUSH     Push a literal value onto the stack
PRINT    Pop a value and write it to stdout
STORE    Pop a value and store it in the variable named by the instruction operand
LOAD     Load the variable named by the operand and push it onto the stack
NEQ      Pop a, pop b, push 1 if b != a, otherwise 0
EQ       Pop a, pop b, push 1 if b == a, otherwise 0
LT       Pop a, pop b, push 1 if b < a, otherwise 0
GT       Pop a, pop b, push 1 if b > a, otherwise 0
JZ       Pop a condition; if it is zero, jump to the operand address
JNZ      Pop a condition; if it is nonzero, jump to the operand address
JNE      Pop a, pop b; if b != a, jump to the operand address
JE       Pop a, pop b; if b == a, jump to the operand address
JMP      Unconditional jump to the operand address
CONCAT   Pop a, pop b; push concatenated string b + a
CT       Create a table with the given name in the table store
STV      Pop value, pop table name, assign table[operand_key] = value
LTV      Pop table name, load table[operand_key] and push it onto the stack
FUNC     Marks the start of a named function in the bytecode stream
CALL     Calls a named function by lookup address, creating a new scope for arguments/locals
RET      Returns execution to the previous call site
INP      Pop a prompt, read a numeric token from stdin, and push the value
```


### Example compilation

```cove
num x = 10;
num y = 5;
print(x + y);
```

This compiles to the following bytecode:

```text
PUSH 10
STORE x
PUSH 5
STORE y
LOAD x
LOAD y
ADD
PRINT
```
