# Cove

Cove is a lightweight, custom stack-based programming language implemented in C++ with Lua-like syntax

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
- function calls: `name(value1, value2)`
- nested argument expressions and nested parentheses inside function calls

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

The source is tokenized, compiled, and executed in the VM.

## Syntax

```cove
num score = 42;
str title = "Cove";

score = score + 8;
print(score);
print(title .. " is ready");

num total = (10 + 5) * 2;

if score > 10 and total < 100 then
    print("ok");
end

while score < 100 do
    score = score + 10;
    print(score);
end

for i = 0, i < 5, i = i + 1 do
    print("tick: " .. i);
end

function greet(name)
    print("Hello, " .. name);
end

greet("Ada");

function add_and_print(a, b)
    print(a + b);
end

add_and_print(10, 5);
add_and_print((2 + 3), (8 / 2));
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

Cove now supports simple function declarations and calls. Function bodies are written between `function ... end`, and a function is invoked by writing its name followed by a parenthesized argument list.

```cove
function greet(name)
    print("hello, " .. name);
end

greet("Cove");

greet("Ada");

function sum_and_print(a, b)
    print(a + b);
end

sum_and_print(10, 5);
sum_and_print((4 * 3), (9 / 3));
```

Function calls can include nested expressions inside the argument list, and arguments are evaluated before the function body runs.

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

## Full-feature example

```cove
num score = 10;
num rounds = 3;
num total = ((score * rounds) % 7) + (score / 2);

str app = "Cove";
str state = "ready";

function greet_player(name, level)
    print("Welcome, " .. name .. " (level " .. level .. ")");
end

function evaluate_rounds(current, bonus)
    print(current + bonus);
end

if score > 5 and rounds < 10 then
    print(app .. " is " .. state);
end

tbl machine = {
  name = "Ada";
  phase = "compile";
  level = 7;
}

if (score > 5 and rounds < 10) or (machine.level == 7) then
    machine.phase = "live";
    print(machine.name .. " :: " .. machine.phase);
    print(total);
    print(machine.level);

    if machine.level ~= 7 then
        print("unexpected");
    end
end

while score < 20 do
    score = score + 2;
    print("looping: " .. score);
end

greet_player("Ada", machine.level);
evaluate_rounds(score, 5);

for i = 0, i < 3, i = i + 1 do
    print("round: " .. i);
end
```

This covers the current language capabilities: variable declarations, reassignment after declaration, arithmetic, comparisons, branching, loops, string building, tables, and function declarations/calls.

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
```

The set of opcodes is executed in order and uses the VM stack plus the variable & table stores.

### Execution model

- `PUSH` places literals or values at the top of the stack
- arithmetic opcodes pop two operands and push one result back
- `STORE` and `LOAD` work with the variable map by name
- function values can be loaded from names or from table entries and then called via `CALL`
- `JZ`, `JNZ`, `JE`, and `JNE` are control-flow instructions that use the program counter
- `CT`, `STV`, and `LTV` manage named tables and keyed values in the current scope chain
- `FUNC`, `CALL`, and `RET` manage function scope and execution flow, including a fresh table scope per call frame
- `CONCAT` accepts strings and numbers, numeric operands are converted to strings before concatenation

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

# PERFORMANCE

The following script demonstrates how Cove's VM handles recursion, arithmetics, memory allocation, and control flow.


```cove
print("--- COVE VM STRESS TEST ---");

tbl config = {
    iterations = 25000;
    seed = 42;
    modulus = 10007;
}

tbl state = {
    acc = 0;
    x = 1;
    y = 2;
}

function complex_math(val)
    num step1 = ((val * 3) % 17) + (val / 2);
    num step2 = step1 - ((val * 5) % 11);
    return step2 + (val * (val - 1) / 2);
end

function mutate_state()
    num temp_x = state.x;
    
    state.x = (state.x * 5) + (state.y * 3) + config.seed;
    state.x = state.x % config.modulus;
    
    state.y = (temp_x * 2) + (state.y * 7) + (config.seed / 2);
    state.y = state.y % config.modulus;
    
    num math_val = complex_math(state.x);
    state.acc = state.acc + math_val;
    
    config.seed = (config.seed + 1) % 100;
    return state.acc;
end

function fib(n)
    if n < 2 then
        return n;
    end
    return fib(n - 1) + fib(n - 2);
end

print("Phase 1: Deep Loop, Table Mutation, & Arithmetic");
for i = 0, i < config.iterations, i = i + 1 do
    mutate_state();
end

print("Phase 1 Complete. Final Accumulator:");
print(state.acc);

print("Phase 2: Deep Recursion (Fibonacci)");
num f_val = fib(24);
print("Phase 2 Complete. Fib(24):");
print(f_val);

print("Phase 3: Heap / String Allocations");
str msg = "A";
for j = 0, j < 12, j = j + 1 do
    msg = msg .. msg;
end
print("Phase 3 Complete. Outputting 4096-char string:");
print(msg);

print("Phase 4: Nested Loops & Logic Branching");
num counter = 0;
num m = 0;
while m < 50 do
    num n = 0;
    while n < 50 do
        if (m % 2) == 0 then
            counter = counter + 1;
        end
        if (m % 2) ~= 0 then
            counter = counter + 2;
        end
        n = n + 1;
    end
    m = m + 1;
end

print("Phase 4 Complete. Logic Counter:");
print(counter);
print("--- TEST FINISHED ---");

```

### Summary

**Compile Time:** 0.14ms avg

**Execution Time:** 74.60ms avg

* **Phase 1 (Iteration & Arithmetic):** 25,000 loop iterations processing table mutations and arithmetic operations.
* **Phase 2 (Recursion):** 150,051 recursive `CALL` and `RET` instructions executed for `fib(24)`.
* **Phase 3 (Allocation):** 12 exponential concatenation cycles resolving a 4,096-character string block.
* **Phase 4 (Control Flow):** 2,500 total inner loop cycles (50 outer × 50 inner) executing conditional branching.