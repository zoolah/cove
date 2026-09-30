# Cove

Cove is a lightweight, custom stack-based programming language implemented in C++ with Lua-like syntax

WASM Powered Playground (write & execute cove code) - [https://zoolah.github.io/cove/web](https://zoolah.github.io/cove/web/index.html)

Documentation Site - https://zoolah.github.io/cove/web/documentation.html

## What it supports

- `num` and `str` values
- arithmetic: `+ - * / %`
- unary arithmetics: `++ --`
- comparisons: `== ~= < >`
- conditionals: `if ... then ... end`
- while loops: `while expression do ... end`
- for loops: `for i = 0, i < 10, i++ do ... end`
- logical chaining: `and`, `or`
- string concatenation: `..`
- tables with dot access: `profile.name`
- tables can hold function values and call them through the table: `obj.fn(...)`
- classes with declared members: `class`, `new`, and `instance::member`
- variable reassignment after declaration: `varname = anyexpression;`
- function declarations: `function name(arg1, arg2) ... end`
- function calls: `name(value1, value2)` and return values with `return expression`
- nested argument expressions and nested parentheses inside function calls
- a fresh variable, table, and class-instance scope for every function call
- string input with `input(prompt)`
- bytecode disassembly with `--verbose`

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

Cove requires a C++17 compiler (it uses `std::variant` and `if` statements with initializers). 

## Quick start

```bash
cove.exe program.txt
cove.exe program.txt --standalone
cove.exe program.txt -s
```

By default, the source is tokenized, compiled, and executed in the VM. With `--standalone` (`-s`), Cove writes a self-contained executable beside the source file, using the same base name with an `.exe` extension. The generated executable contains the VM and compiled bytecode and does not need the source file or the Cove compiler to run.

Command line:

```text
cove.exe [--verbose | -v] [--standalone | -s] <filename>
cove.exe --help | -h
```

`--verbose` prints the disassembled bytecode before execution, followed by the time spent compiling and executing. Any error, whether it comes from the tokenizer, the compiler, or the VM, is printed to standard error as `Fatal Error: <message>` and the process exits with status 1. A program that compiles to zero instructions is rejected with `Compilation failed`.



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

A declaration must always include an initializer. Cove is dynamically typed: every value is either a number, a string, or a function reference, and the `num`/`str` keyword is only declaration syntax. It is not checked against the value being stored, so `num x = "text";` is accepted. A variable can be reassigned to a value of a different type at any time.

Numbers are 64-bit floating point values. `+ - * /` require two numbers, and `+` does not concatenate strings (use `..`). Division by zero raises `Division by zero`. `%` is a floating point remainder (`fmod`) and does not raise an error for a zero divisor. When a number is printed or concatenated, integral values are written without a decimal point, and other values are written with up to 15 fractional digits with trailing zeros removed.

Assigning to an existing variable uses the same form without the type keyword:

```cove
num count = 1;
count = count + 1;
```

## Unary Arithmetics

```
num x = 1;
x = x++;

print(x);

x = x--;
print(x);

x++;
print(x);

print(x++);
print(--x);
print(x);

```

With prefix notation (`++x` or `--x`), the variable is updated first, and the expression evaluates to the new value. With postfix notation (`x++` or `x--`), the expression evaluates to the current value before the variable updates.

An assignment of a variable to its own postfix expression (`x = x++;` or `x = x--;`) is compiled without the final store, so the update is kept. This differs from C and Java, where the assignment would restore the old value. The example above prints `2`, `1`, `2`, `2`, `2`, `2`.

`++` and `--` operate on plain variables only. They can be used as a standalone statement (`x++;`), inside an expression, and as the iterator of a `for` loop.

## Expressions

Operators from highest to lowest precedence. Operators on the same level are applied left to right, and parentheses group as usual.

```text
1.  *   /   %
2.  +   -   ..
3.  ==  ~=  <   >
4.  and  (also &)
5.  or   (also |)
```

Note that `..` shares a level with `+` and `-`. `"total: " .. 1 + 2` is evaluated as `("total: " .. 1) + 2` and fails at runtime, so write `"total: " .. (1 + 2)`.

`..` accepts strings and numbers on either side and produces a string. Numbers are formatted with the same rules as `print`.

Comparisons produce `1` for true and `0` for false. `==` and `~=` accept two numbers or two strings; comparing a number with a string is a runtime error. `<` and `>` accept numbers only. There is no `<=` or `>=`: the tokenizer reads `<=` as `<` followed by a stray `=`, and the `=` is silently dropped inside an expression, so `a <= b` behaves as `a < b`. Combine comparisons instead: `a < b or a == b`.

`and` is compiled to a multiplication and `or` to an addition. Both operands are always evaluated (there is no short-circuiting) and the result is not normalized to `0` or `1`: `2 and 3` is `6` and `1 or 1` is `2`. Both operators require numbers, which is what comparisons produce. Since conditions only test for zero, this only matters if a logical result is printed or reused arithmetically.

Conditions in `if`, `while`, and `for` must evaluate to a number. `0` is false and any other number is true. A string condition raises `JZ requires a number condition`.

Unary minus is not supported. The tokenizer produces a binary subtraction token for every `-`, so `-5` fails at runtime with `Stack underflow`. Write `0 - 5` instead.

An expression can contain:

```text
5, 2.5            number literals
"text", 'text'    string literals (no escape sequences)
name              a variable, or a function name (evaluates to a function value)
t.key             a table field read
inst::member      a class instance member read
f(a, b)           a function call
t.fn(a, b)        a call through a table field
input(prompt)     a line of input, as a string
++x  --x  x++  x--
```

## If statements 

```cove
num score = 42;

if score > 10 then
    print("high score");
end
```

There is no `else` or `elseif`. Blocks can be nested. The compiler locates `then` by searching forward from the `if` for the first token spelled `then`, and the block continues until the matching `end`. Statements inside an `if` block run in the enclosing scope.

## While loops

A `while` loop evaluates its condition before every iteration and stops when the condition becomes false. 

```cove
num count = 0;

while count < 3 do
    print(count);
    count = count + 1;
end
```

There is no `break` or `continue`.

## For loops

Cove supports a C-style loop header with initializer, condition, and iterator update separated by commas:

```cove
for i = 0, i < 5, i++ do
    print("iteration: " .. i);
end
```

The loop initializes `i`, evaluates the condition before each pass, and runs the iterator expression after each iteration.

Important: For loops iterator MUST use a unary operator like ++, --, or *coming soon* +=, or -=.

The iterator is compiled as an expression, not as a statement. An assignment such as `i = i + 1` therefore does not update `i` (the `=` is dropped) and the loop never terminates. The loop variable is stored in the current scope and remains defined after the loop ends. The header is split at the first two commas that follow the `for` keyword, regardless of nesting, so a comma-separated call inside the initializer or condition is not supported.

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

Arguments are passed left to right and bound to the parameters in order. The VM does not check argument counts. Passing fewer arguments than parameters consumes unrelated values from the operand stack or raises `Stack underflow`, and extra arguments are left on the stack.

Functions are global and are registered before the program starts running, so a function may be called above its definition. Defining two functions with the same name keeps the last one. A function defined inside another function body is still registered globally at startup. Recursion is supported. `return` outside a function raises `Call stack underflow on RET`.

The semicolon after a call statement is optional (`greet("Ada")` is accepted), and a call used as a statement leaves its return value on the operand stack.

A function name used as an expression evaluates to a function value. Function values can be stored in table fields and called through the table with `table.field(...)`. They cannot be stored in a variable and called as `variable(...)`, since a call by name always looks the name up in the function registry. `print` does not write anything for a function value, and `..` rejects one.

## Classes

```cove
class Person {
     str name;
     num health;
}

new Person() person;

person::health = 100;
person::name = "Bryan";

print(person::name .. " has " .. person::health .. " health!");
```

A class body contains only member declarations of the form `num name;` or `str name;`. There are no methods and no constructors, so the parentheses in `new Class() instance;` are required but their contents are not compiled. Members are initialized to `0` (`num`) or `""` (`str`); the keyword selects that default and does not restrict later assignments.

Members are read and written with `instance::member`. Assigning to a member that was not declared raises `Member not found in class instance`. Each instance holds its own copy of the class members.

Class definitions execute at runtime, in program order, and are global. `new` fails with `Class not found` if the class definition has not executed yet. Instances follow the same scope rules as tables (see Scope).

## Input

`input(prompt)` prints its string prompt, reads one token from standard input, and returns it as a string. Cove does not implicitly convert input strings to numbers, so using an input result in arithmetic raises a runtime error.

```cove
str name = input("What is your name? ");
print("Hello, " .. name);
```

A token is a run of characters up to the next whitespace, so `input` returns a single word, not a whole line. The prompt must be a string, and `input` takes exactly one argument. `input` is reserved and cannot be redefined, and it can only appear inside an expression (a bare `input("x");` statement is a compile error).

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

Further rules:

- Every entry is `key = expression;` and must end with a semicolon. There is no semicolon after the closing `}`.
- Keys are identifiers accessed with `.`. There is no bracket indexing, and a table value cannot itself be a table.
- A table is not a value. It cannot be assigned to a variable or passed to a function; it is referenced by name and resolved through the scope stack.
- Assigning to a key that does not exist creates it. Reading a key that does not exist raises `Key not found in table`.
- Declaring a table name that already exists in the current scope raises `Attempt to redefine table`. Declaring it in a deeper scope (a function call) is allowed and shadows the outer table.
- Tables occupy their own namespace, separate from variables. A table and a variable may share a name.

## Scope

Cove keeps one scope stack per kind of named storage (variables, tables, class instances), and a function call pushes one entry onto each. The global scope is the bottom entry and is never removed.

- Reading a name searches from the innermost scope outward. Because the stack is built from calls and not from source nesting, scoping is dynamic: a function can read the local variables of whatever function called it.
- Writing a variable (`STORE`) always writes into the innermost scope. Assigning to a name inside a function creates a local variable and never modifies a global of the same name.
- `if`, `while`, and `for` blocks do not create scopes.
- Function parameters are ordinary locals of the call.

```cove
num x = 1;

function change()
    x = 2;
    print(x);
end

change();
print(x);

function show()
    print(secret);
end

function run()
    num secret = 7;
    show();
end

run();
```

This prints `2`, `1`, then `7`.

## Lexical rules

Source text is free-form; whitespace only separates tokens. Newlines have no meaning, and statements end with `;` except for the block forms (`if`, `while`, `for`, `function` end with `end`; `class` and `tbl` end with `}`) and call statements, whose semicolon is optional.

- Identifiers start with a letter or `_` and continue with letters, digits, or `_`.
- Number literals are digits with at most one decimal point. A second decimal point raises `Malformed number`. Negative literals do not exist (see Expressions).
- String literals use `"` or `'` and end at the next matching quote. There are no escape sequences, so a string cannot contain its own quote character, but may contain the other kind and may span lines. An unterminated string extends to the end of the file.
- There is no comment syntax. `--` is always the decrement token.
- Characters the tokenizer does not recognize (for example `!`, `[`, `]`, `#`, `@`, and a single `:`) are skipped without an error.
- Reserved words: `print num str if then end while do for function return class new tbl input and or`. The words `then`, `do`, and `end` are located by spelling anywhere in the token stream, so they cannot be used as variable names.

## Errors

Errors are C++ exceptions that abort the program. They carry a message but no line number; compile errors that report a position use a token index.

Syntax errors are raised by the compiler before anything runs. Typical messages:

```text
Unexpected token '<t>' at root level (index N)      a statement does not start with an identifier
Unknown identifier: '<t>' at token index N          the statement matches no known form
Missing closing semicolon ';' ...                   a statement is not terminated
Missing 'then' keyword after if statement
Missing 'end' keyword for if statement / function '<name>'
Expected 'do' after 'while'
Expected ',' after for loop initializer / condition
Missing closing parenthesis for print / call to '<name>'
Expected '=' in variable declaration for '<name>' ...
input() can only be used in an expression
Input expects 1 argument
Malformed number                                    raised by the tokenizer
```

Runtime errors are raised by the VM while executing:

```text
Stack underflow
ADD/SUB/MUL/DIV/MOD requires numbers
Division by zero
Concatenation (..) requires string or number operands
Cannot EQ/NEQ number and string
Error: Attempt to compare non-number with < / >
JZ requires a number condition
Undefined variable: <name>
Undefined function: <name>
Attempt to call a non-function value
Call stack underflow on RET
Table not found: <name>
Key not found in table '<name>': <key>
Attempt to redefine table: <name>
Class not found: <name>
Class instance not found: <name>
Member not found in class instance '<name>': <member>
```



# Internals

## Pipeline

```text
source text
    |  Compiler::tokenize            std::vector<Token>
    v
tokens
    |  Compiler::compile             statements -> bytecode
    |    compile_block               statement dispatcher
    |    evaluate_expression         expressions -> bytecode (shunting-yard)
    v
std::vector<Instruction>
    |  Machine::run                  fetch / dispatch loop
    v
output
```

Source layout:

```text
src/
├── entry.cpp                  command line driver, timing, --verbose
├── shared/
│   └── structs.hpp            Opcode, Value, Instruction, TokenType, Token, Stack
├── compiler/
│   ├── compiler.hpp           declarations, helpers, statement dispatcher, compile()
│   ├── components/
│   │   ├── tokenizer.cpp      Compiler::tokenize
│   │   ├── evaluator.cpp      Compiler::evaluate_expression
│   │   └── printer.cpp        Compiler::print_bytecode (disassembler)
│   └── statements/
│       ├── basic.cpp          print, declarations, assignment, calls, return
│       ├── cflow.cpp          if, while, for
│       └── definitions.cpp    function, class, tbl, new
└── vm/
    ├── machine.hpp            Machine, run loop, ops:: declarations
    └── ops/
        ├── stack.cpp          PUSH POP DUP SWAP
        ├── arithmetic.cpp     ADD SUB MUL DIV MOD CONCAT
        ├── io.cpp             PRINT INP
        ├── vars.cpp           STORE LOAD
        ├── cflow.cpp          comparisons, jumps, FUNC CALL RET
        ├── tables.cpp         CT STV LTV
        └── classes.cpp        CDEF CNUM CSTR INSTC CLOAD CSTORE
```

## Data structures

All shared types live in `shared/structs.hpp`.

**`Value`** is a tagged union: `std::variant<double, std::string, FunctionReference>`, where `FunctionReference` holds a function name. A default-constructed `Value` is the number `0.0`. Accessors are `is_number()`, `is_string()`, `is_function()` and `as_number()`, `as_string()`, `as_function()`; an `as_*` call on the wrong alternative throws `std::bad_variant_access`. Function values are created with `Value::function(name)`. Values are copied by value everywhere.

**`Instruction`** is `{ Opcode op; Value operand; }`. Instructions with no operand carry the default operand `0.0`. Operands hold literals, variable/table/class names (as strings), and absolute jump targets (as numbers).

**`Token`** is `{ TokenType type; std::string value; }`. The value is filled for identifiers, numbers, and strings.

**`Stack`** is a `std::vector<Value>` wrapper with `push(Value)`, `pop()` (throws `Stack underflow` when empty), and `size()`.

Token types:

```text
IDENTIFIER   names and keywords        NUMBER   number literal      STR      string literal
LP  (        RP  )                     LB  {    RB  }
ADD +        SUB -                     MUL *    DIV /               MOD %
SEQ =        SC  ;                     DOT .    COMMA ,             DCOLON ::
EQ  ==       NOTEQ ~=                  LT  <    GT  >
AND and, &   OR  or, |                 CONCAT ..
INC ++       DEC --                    EOF      (defined, never produced)
```

## Compiler

### Tokenizer

`Compiler::tokenize` makes a single left-to-right pass. Two-character tokens (`++ -- :: .. ~= ==`) are matched before their one-character prefixes, and `and` / `or` are recognized after an identifier is read; every other word, including all keywords, is an `IDENTIFIER`. Keywords are recognized later by their spelling.

### Statement compiler

`Compiler::compile` calls `compile_block` repeatedly until every token is consumed. `compile_block(t, pos, bytecode)` compiles one statement starting at `t[pos]` and returns the index of the next statement. The first token must be an identifier; the dispatcher then applies the following rules in order, and the first match wins:

```text
 1  print                        compile_print
 2  num | str                    compile_decl
 3  if                           compile_if
 4  tbl                          compile_table_def
 5  while                        compile_while
 6  for                          compile_for
 7  function                     compile_function
 8  class                        compile_class
 9  new                          compile_new
10  id ::  (3+ tokens follow)    compile_member_assign
11  input                        error: only valid in an expression
12  id (                         compile_call
13  id . id (                    compile_method_call
14  id .                         compile_table_assign
15  id ++ | id --                compile_incdec_stmt
16  id =                         compile_assign
17  return                       compile_return
18  anything else                error: Unknown identifier
```

Block statements call `compile_until_end`, which compiles statements until the `end` keyword and returns its index, so blocks nest naturally. Forward jumps are emitted with a placeholder operand and patched once the block has been compiled. All jump operands are absolute instruction indices.

Bytecode emitted for each statement (`<e>` stands for the bytecode of expression `e`):

```text
num x = e;                  <e>  STORE x
x = e;                      <e>  STORE x           (omitted for x = x++; and x = x--;)
print(e);                   <e>  PRINT
x++;                        LOAD x  DUP  PUSH 1  ADD  STORE x  POP
f(a, b);                    <a>  <b>  CALL f
t.m(a, b);                  <a>  <b>  PUSH t  LTV m  CALL
return;                     PUSH 0  RET
return e;                   <e>  RET

if c then B end             <c>  JZ end  B
while c do B end            L: <c>  JZ end  B  JMP L
for v = i, c, s do B end    <i>  STORE v  L: <c>  JZ end  B  <s>  JMP L

function f(p, q) B end      JMP after  FUNC f  STORE q  STORE p  B  PUSH 0  RET
                            (parameters are stored in reverse: the last argument is on top)

tbl t = { k = e; }          CT t  PUSH t  <e>  STV k
t.k = e;                    PUSH t  <e>  STV k

class C { num a; str b; }   CDEF C  CNUM a  CSTR b
new C() i;                  PUSH C  INSTC i
i::m = e;                   <e>  PUSH i  PUSH m  CSTORE
```

In every case `end` is the index just past the last instruction of the construct, `after` is the index just past the function's trailing `RET`, and `L` is the index of the first condition instruction.

### Expression evaluator

`Compiler::evaluate_expression(tokens)` compiles one infix expression in two passes.

**Pass 1** converts the tokens to reverse Polish notation with the shunting-yard algorithm. All binary operators are left-associative. Constructs that need more than one token are collapsed into a single marker token:

- `name(args)` and `table.name(args)` become `CALL:name` / `CALL:table.name`
- `input(arg)` becomes `INPUT`
- `a.b` and `a::b` become one identifier, `a.b` / `a::b`
- `x++`, `x--`, `++x`, `--x` become the identifier followed by `POSTINC`, `POSTDEC`, `PREINC`, or `PREDEC`

The argument token lists of calls are kept in a side table keyed by the marker's position and are compiled recursively in pass 2. Tokens that match no rule (`=`, `,`, stray `++`) are dropped without an error, and an unmatched `(` is reported in pass 2 as `Unsupported operator`.

**Pass 2** walks the RPN and emits bytecode:

```text
5, "s"          PUSH value
name            LOAD name
t.k             PUSH t  LTV k
i::m            PUSH i  CLOAD m
a op b          <a>  <b>  op       ADD SUB MUL DIV MOD CONCAT EQ NEQ LT GT
a and b         <a>  <b>  MUL
a or b          <a>  <b>  ADD
f(a, b)         <a>  <b>  CALL f
t.f(a, b)       <a>  <b>  PUSH t  LTV f  CALL
input(p)        <p>  INP
++x             LOAD x  PUSH 1  ADD  DUP  STORE x       leaves the new value
x++             LOAD x  DUP  PUSH 1  ADD  STORE x       leaves the old value
--x, x--        same with SUB
```

## Virtual machine

### Machine state

```cpp
class Machine {
public:
    Stack stack;                                                              // operand stack
    std::vector<umap<std::string, Value>> scopes;                             // variable scopes
    std::vector<umap<std::string, umap<std::string, Value>>> tablescopes;     // table scopes
    umap<std::string, umap<std::string, Value>> cdefs;                        // class definitions
    std::vector<umap<std::string, umap<std::string, Value>>> cscopes;         // class instance scopes
    std::string curr_class_def = "";                                          // class being defined
    std::vector<uint64_t> call_stack;                                         // return addresses
    umap<std::string, uint64_t> function_addresses;                           // function entry points
    uint64_t pc = 0;                                                          // program counter
};
```

`umap` is an alias for `std::unordered_map`.

```text
stack                Operand stack shared by the whole program. Instructions pop their inputs and push results.
scopes               Stack of variable maps (name -> Value). Index 0 is global; every CALL pushes one, RET pops it.
tablescopes          Parallel stack of table maps (table name -> (key -> Value)).
cscopes              Parallel stack of class instance maps (instance name -> (member -> Value)).
cdefs                Class definitions (class name -> member defaults). Global, not scoped.
curr_class_def       Name set by CDEF so that the following CNUM / CSTR know which class they extend.
call_stack           Return addresses pushed by CALL and popped by RET.
function_addresses   Function name -> index of the instruction after its FUNC marker.
pc                   Index of the instruction being executed.
```

`scopes`, `tablescopes`, and `cscopes` always have the same depth: they are pushed together by `CALL` and popped together by `RET`. Lookups walk them from the back (innermost) to the front (global); writes to variables go to the back only. `CT` creates a table in the back scope, while `STV` and `CSTORE` modify the nearest existing table or instance. `INSTC` copies the class definition's member map into the back instance scope, so instances are independent.

### Execution

`Machine::run(bytecode)` performs the following:

1. Resets `pc`, `call_stack`, `function_addresses`, `cdefs`, and all three scope stacks, then pushes one empty global entry onto each scope stack. The operand stack is not cleared.
2. Scans the bytecode once and records `function_addresses[name] = index + 1` for every `FUNC` instruction. This is why functions can be called before they are defined.
3. Loops while `pc < bytecode.size()`: fetch the instruction, set `next_pc = pc + 1`, dispatch to the handler for its opcode, then set `pc = next_pc`. Only control-flow instructions change `next_pc`.

There is no halt instruction; a program ends when `pc` moves past the last instruction, which is also what a jump to `bytecode.size()` does. Errors are `std::runtime_error` exceptions that propagate out of `run`.

Instructions are dispatched to one handler per opcode group. Each handler has the signature `void(Machine&, Instruction&)`, except `cflow`, which also takes `uint64_t& next_pc`.

```text
ops::stack        PUSH POP DUP SWAP
ops::arithmetic   ADD SUB MUL DIV MOD CONCAT
ops::io           PRINT INP
ops::vars         STORE LOAD
ops::cflow        EQ NEQ LT GT  JZ JNZ JE JNE JMP  FUNC CALL RET
ops::tables       CT STV LTV
ops::classes      CDEF CNUM CSTR INSTC CLOAD CSTORE
```

### Calling convention

A call sequence is: the caller pushes the arguments left to right, then executes `CALL`. `CALL` resolves the callee, pushes a new entry onto `scopes`, `tablescopes`, and `cscopes`, pushes `next_pc` onto `call_stack`, and jumps to the function's address. The function prologue is one `STORE` per parameter in reverse order, which pops the arguments into the new scope. `RET` pops the return value, pops the three scope stacks (never below the global entry), restores `pc` from `call_stack`, and pushes the return value again for the caller.

`CALL` has two forms. With a string operand it calls that function by name. Without an operand it pops the callee from the stack, which must be a function value or a string naming a function; this is the form used by `table.fn(...)`. In both forms the callee is looked up in `function_addresses` and an unknown name raises `Undefined function`.

The compiler emits `POP` only after an increment/decrement statement. The result of a call statement, and the value of a `for` loop's iterator expression, stay on the operand stack.

### Opcode reference

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
INP      Pop a prompt, read a token from stdin, and push it as a string
DUP      Pops a, then pushes a twice
SWAP     Pops a, pops b, pushes a, pushes b
CDEF     Define a class named by the operand and make it the current class definition
CNUM     Add a numeric member (default 0) named by the operand to the current class definition
CSTR     Add a string member (default "") named by the operand to the current class definition
INSTC    Pop a class name; create an instance named by the operand from that class in the current scope
CLOAD    Pop an instance name, load member[operand] of that instance and push it onto the stack
CSTORE   Pop a member name, pop an instance name, pop a value, assign instance.member = value
```

Details that the summary above does not capture:

```text
PUSH     The operand may be a number or a string.
ADD..MOD Both operands must be numbers. DIV raises "Division by zero" for a zero divisor. MOD uses fmod
         and yields NaN for a zero divisor.
CONCAT   Accepts strings and numbers. Integral numbers are written without a decimal point, other numbers
         with up to 15 fractional digits, trailing zeros removed.
PRINT    Numbers and strings are written followed by a newline. A function value writes nothing.
EQ NEQ   Both operands must be numbers or both strings.
LT GT    Both operands must be numbers.
JZ JNZ   The condition must be a number. Jump targets are the operand converted to an unsigned integer.
JE JNE   Same operand type rules as EQ / NEQ. JNZ, JE, and JNE are implemented by the VM but never emitted
         by the compiler.
LOAD     Searches the variable scopes innermost to outermost. If no variable matches but a function with
         that name exists, pushes a function value. Otherwise raises "Undefined variable".
STORE    Always writes to the innermost variable scope.
CT       Raises "Attempt to redefine table" if the name already exists in the innermost table scope.
STV      Searches the table scopes innermost to outermost; creates the key if it does not exist.
LTV      Raises "Key not found in table" if the key does not exist.
FUNC     A no-op when executed. The function's address is registered by the pre-scan in Machine::run.
CALL     With a string operand, calls by name. With no operand, pops the callee (function value or string).
RET      Raises "Call stack underflow on RET" outside of a call.
INP      Writes the prompt without a newline, then reads one whitespace-delimited token.
DUP      SWAP raises "SWAP requires two values" when fewer than two values are on the stack.
CLOAD    Searches the instance scopes innermost to outermost; the member must exist.
CSTORE   The member must exist on the instance.
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

A function definition and call:

```cove
function add(a, b)
    return a + b;
end

add(1, 2);
```

compiles to:

```text
 0: JMP      10
 1: FUNC     add
 2: STORE    b
 3: STORE    a
 4: LOAD     a
 5: LOAD     b
 6: ADD
 7: RET
 8: PUSH     0
 9: RET
10: PUSH     1
11: PUSH     2
12: CALL     add
```

`JMP 10` skips the body during normal execution. The `PUSH 0` / `RET` pair at 8-9 is the implicit return that every function receives. The call at 12 jumps to index 2, the address recorded for `add`.

### Disassembler

`--verbose` runs `Compiler::print_bytecode` before execution. Each line is the instruction index, the mnemonic, and the operand if it has one. Instructions inside a function (from `FUNC` to its final `RET`) are indented by two spaces, and the implicit `PUSH 0` / `RET` pair is kept inside that indentation. The listing is followed by `Compile took` and `Execute took` timings in milliseconds.
