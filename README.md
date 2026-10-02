```markdown
# Cove

Lightweight stack-based language in C++ with Lua-like syntax.

- Playground: https://zoolah.github.io/cove/web
- Docs site: https://zoolah.github.io/cove/web/documentation.html

## Features

- `num` and `str` values
- arithmetic `+ - * / %`
- unary `++ --`
- comparisons `== ~= < >`
- logical `and` `or` (also `&` `|`)
- string concatenation `..`
- `if ... then ... end`
- `while ... do ... end`
- `for i = 0, i < 10, i++ do ... end`
- tables with `.` access
- tables can hold function values and call them as `obj.fn(...)`
- classes with members, constructors, and member functions
- functions with `return`
- `input(prompt)`
- bytecode listing with `--verbose`
- standalone executable with `--standalone`

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Requires a C++17 compiler. The executable is in `build/Release/cove.exe`.

## Usage

```
cove [--verbose|-v] [--standalone|-s] <filename>
cove --help|-h
```

`--standalone` or `-s` writes a self-contained executable next to the source using the same base name with an `.exe` extension. The generated file holds the VM and the compiled bytecode.

`--verbose` or `-v` prints the bytecode before execution along with compile and execute times.


---

# Language

## Variables

```cove
num x = 5;
str name = "Ada";
x = x + 1;
print(x);
```

Declarations use `num name = expr` or `str name = expr`. Numbers are 64-bit floating point. Assignment after declaration uses `name = expr`.

## Operators

Highest to lowest precedence, left to right:

```
* / %
+ - ..
== ~= < >
and (&)
or  (|)
```

`..` concatenates numbers and strings. Conditions treat `0` as false and any other number as true.

`++` and `--` work as prefix or postfix on plain variables. They can stand alone, appear in expressions, or serve as the iterator in a `for` loop.

## Control flow

```cove
if score > 10 then
    print("high");
end

while count < 3 do
    print(count);
    count = count + 1;
end

for i = 0, i < 5, i++ do
    print(i);
end
```

No `else`, `elseif`, `break`, or `continue`. The `for` header is initializer, condition, and iterator expression separated by commas. The iterator must update the loop variable (normally `i++` or `i--`).

## Functions

```cove
function add(a, b)
    return a + b;
end

print(add(1, 2));
```

Functions are global and registered before the program runs, so calls may appear above the definition. A missing `return` produces `0`. Arguments are pushed left to right. Function values can be stored in table fields and called as `table.field(...)`.

## Tables

```cove
tbl profile = {
    name = "Ada";
    role = "eng";
}

print(profile.name);
profile.role = "arch";

tbl obj = { fn = greet; }
obj.fn("hi");
```

Tables live in their own scope stack parallel to variables. Keys are identifiers only. Reading a missing key raises an error. Writing a missing key creates it. Tables are not values that can be assigned or passed around.

## Classes

```cove
class Person {
    str name;
    num health;

    Person(n, h)
        this::name = n;
        this::health = h;
    end

    getName()
        return this::name;
    end
}

new Person("Bryan", 100) p;
print(p::getName());
p::health = 90;
```

Fields are declared with `num` or `str` and start at `0` or `""`. The constructor has the same name as the class. `new Class(args) instance` creates the instance. Inside constructors and member functions use `this::member`. Outside use `instance::member`. Each instance holds its own copy of the fields.

## Input

```cove
str name = input("Name? ");
print("hi " .. name);
```

`input` prints the prompt, reads one whitespace-delimited token, and returns it as a string. It may only appear inside an expression.

## Scope

The VM keeps separate scope stacks for variables, tables, and class instances. Every function, constructor, or member-function call pushes a new entry on each stack. Lookups walk from the innermost scope outward. Stores always write into the innermost scope. `if`, `while`, and `for` blocks do not push scopes.

## Lexical rules

Source is free-form. Whitespace separates tokens. Most statements end with `;` (optional after calls). Block forms end with `end` or `}`.

Identifiers start with a letter or `_`. Number literals allow one decimal point. Strings use `"` or `'` with no escape sequences. There is no comment syntax (`--` is always decrement). Unrecognized characters are skipped. Reserved words: `print num str if then end while do for function return class new tbl input and or`.

---

# Internals

## Pipeline

```
source text
  -> Compiler::tokenize
  -> Compiler::compile
  -> Machine::run
```

## Source layout

```
src/
├── entry.cpp                 command line, timing, standalone check
├── shared/structs.hpp        Opcode, Value, Instruction, Token, Stack
├── compiler/
│   ├── compiler.hpp          statement helpers and compile_block
│   ├── components/           tokenizer, evaluator, printer
│   └── statements/           basic, cflow, definitions
├── vm/
│   ├── machine.hpp           run loop and scope state
│   └── ops/                  stack, arithmetic, io, vars, cflow, tables, classes
└── standalone/               embed and extract bytecode for -s
```

## Data

`Value` is a `std::variant<double, std::string, FunctionReference>`.  
`Instruction` holds an `Opcode` and a `Value` operand.  
`Token` holds a `TokenType` and a string value.  
`Stack` is a `std::vector<Value>` with `push` and `pop`.

## Compiler

`tokenize` walks the source left to right. Two-character tokens (`++ -- :: .. ~= ==`) are matched first.

`compile` repeatedly calls `compile_block` until the tokens are finished. `compile_block` examines the first identifier and selects the matching statement compiler (`compile_print`, `compile_decl`, `compile_if`, `compile_table_def`, `compile_while`, `compile_for`, `compile_function`, `compile_class`, `compile_new`, member call or assign, ordinary call, table assign, inc/dec statement, assign, or return).

Expressions are handled by `evaluate_expression`. It converts infix tokens to reverse Polish form then emits bytecode. Calls, `input`, dotted access, member access, and `++`/`--` are turned into markers that expand in the second pass.

## Virtual machine

`Machine` holds:

- operand `stack`
- `scopes` for variables
- `tablescopes` for tables
- `cscopes` for class instances
- `class_type_scopes` mapping instance names to class names
- `cdefs` for class definitions
- `function_addresses` and `member_function_addresses`
- `call_frames`
- program counter `pc`

`run` clears state, pushes one global entry on each scope stack, records every `FUNC` address, then loops while `pc` is inside the bytecode. Each instruction is handled by the matching function in `ops` (`stack`, `arithmetic`, `io`, `vars`, `cflow`, `tables`, `classes`). Control-flow and class instructions may change the next `pc`.

A call pushes a new scope entry on every stack, records the return address, and jumps to the function or method body. `RET` restores the previous scopes and return address and leaves the result on the stack.

## Opcodes

```
ADD SUB MUL DIV MOD
PUSH POP DUP SWAP
PRINT INP
STORE LOAD
EQ NEQ LT GT
JZ JMP FUNC CALL RET
CONCAT
CT STV LTV
CDEF CNUM CSTR INSTC CLOAD CSTORE
CONSTRUCTOR CCONSTRUCTOR MFUNC CMFUNC
```

## Example

```cove
num x = 10;
num y = 5;
print(x + y);
```

```
PUSH 10
STORE x
PUSH 5
STORE y
LOAD x
LOAD y
ADD
PRINT
```

```cove
function add(a, b)
    return a + b;
end
add(1, 2);
```

```
 0: JMP 10
 1: FUNC add
 2: STORE b
 3: STORE a
 4: LOAD a
 5: LOAD b
 6: ADD
 7: RET
 8: PUSH 0
 9: RET
10: PUSH 1
11: PUSH 2
12: CALL add
```

`JMP 10` skips the function body on ordinary execution. The final `PUSH 0 RET` is the implicit return. The `CALL` jumps to the address recorded for `add`.
```