# C Compiler Front-End (Frozen Specification v1.0)

A robust, production-quality compiler front end for a defined subset of the C programming language, implemented in C99 according to the frozen **C Compiler Front-End Language & Semantic Specification Version 1.0**.

---

## 1. Project Purpose & Scope

The purpose of this project is to implement a complete front-end compiler pipeline that takes a `.c` source file, performs lexical analysis, parses it into an Abstract Syntax Tree (AST), constructs a scoped symbol table, and enforces comprehensive semantic type checking, control flow analysis, and I/O format verification.

> [!NOTE]
> **Accurate Status Description**:
> **Compiler front-end for the frozen C subset defined in Language & Semantic Specification v1.0.**
> The current front end ends after semantic validation. It does not include intermediate code generation (IR), optimization, or backend machine code emission.

---

## 2. Supported C Subset vs. Excluded Features

### Supported Features
- **Data Types**: `int` (signed 32-bit integer), `float` (single/double precision floating point), `char` (character), `void` (function return type only).
- **Program Structure**: External declarations, `int main()` parameterless entry point, user-defined functions with typed parameters (`int`, `float`, `char`).
- **Declarations & Variables**: Scalar variables with optional initializers, 1-D arrays with positive integer literal dimensions (`type name[N];`).
- **Control Flow**: `if`, `if-else`, `while`, `do-while`, `for (init; cond; update)` with omitted condition support (`for(;;)`), `break`, `continue`, `return`.
- **Operators & Expressions**:
  - Primary: Identifiers, integer literals, float literals, char literals, string literals, parenthesized expressions.
  - Postfix & Prefix: `++`, `--` (on modifiable `int`/`float` lvalues).
  - Unary: `+`, `-`, `!` (logical NOT on booleans), `&` (strictly restricted to `scanf` destination notation).
  - Casts: Explicit C-style casts `(int)`, `(float)`, `(char)`.
  - Multiplicative & Additive: `*`, `/`, `%` (integers only), `+`, `-`.
  - Relational & Equality: `<`, `>`, `<=`, `>=`, `==`, `!=` (produces internal boolean).
  - Logical: `&&`, `||` (requires boolean operands).
  - Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=`.
- **Type System & Conversions**:
  - Implicit widening: `int` $\rightarrow$ `float` allowed.
  - All other implicit cross-category conversions forbidden (e.g. `float` $\rightarrow$ `int`, `char` $\rightarrow$ `int`).
  - Explicit casts permitted among `int`, `float`, and `char`.
- **I/O Formatted Operations**:
  - `printf`: format string with `%d`, `%f`, `%c`, `%s`, and escaped `%%`.
  - `scanf`: format string with `%d`, `%f`, `%c` and `&identifier` destinations (`%s` is not supported in `scanf`).
- **Scope & Symbol Management**: Global scope, function scopes, nested block scopes, and inner-block shadowing.

### Intentionally Excluded Constructs
Per the frozen specification baseline, the following features are unsupported and cleanly rejected:
- General pointers and pointer arithmetic (the ampersand `&` is recognized *only* for `scanf` destinations).
- Structs, unions, enums, and typedefs.
- Multi-dimensional arrays (`int a[3][3]`).
- Function pointers and variadic user functions.
- Dynamic memory allocation (`malloc`, `free`).
- Preprocessor directives (`#include`, `#define`).
- Hexadecimal, octal, binary, and scientific floating-point literals.

---

## 3. Compiler Pipeline Architecture

```
C Source File (.c)
       │
       ▼
[ Lexical Analyzer ]  ──────▶  Token Stream (include/token.h)
(src/lexer.c)                  Tracks line/column, strips comments/whitespace
       │
       ▼
[ Recursive Parser ]  ──────▶  Abstract Syntax Tree (include/ast.h)
(src/parser.c)                 LL(1) predictive descent with 3-token lookahead
       │
       ▼
[ Symbol Table & Scopes ] ──▶  Scope Tree & Symbol Signatures
(src/symbol_table.c)           Nested scopes, declarations, predefined printf/scanf
       │
       ▼
[ Semantic Type System ] ───▶  Type Rules & Cast Matrix
(src/type_system.c)            Conversion checks, operator validity
       │
       ▼
[ Semantic Analyzer ] ──────▶  Semantic Diagnostics & Validated AST
(src/semantic.c)               Checks SEM-01 through SEM-20, I/O formats, ampersand
       │
       ▼
Compilation Summary / Result
```

---

## 4. Project Directory Structure

```
C-Compiler/
├── src/                          # Front-end implementation sources
│   ├── main.c                    # CLI driver and pipeline orchestrator
│   ├── token.c                   # Token structures and diagnostics
│   ├── lexer.c                   # Lexical scanner
│   ├── ast.c                     # AST nodes and allocation
│   ├── parser.c                  # Recursive descent syntax parser
│   ├── symbol_table.c            # Scoped symbol tables
│   ├── type_system.c             # Semantic type rules and conversion matrices
│   └── semantic.c                # Full semantic analyzer, I/O checks, SEM-01..SEM-20
│
├── include/                      # Public module headers
│   ├── token.h
│   ├── lexer.h
│   ├── ast.h
│   ├── parser.h
│   ├── symbol_table.h
│   ├── type_system.h
│   └── semantic.h
│
├── tests/                        # Verification and test suites
│   ├── lexer/                    # Lexer test inputs and expected outputs
│   ├── parser/                   # AST, expression, statement, and program parser tests
│   ├── semantic/                 # Symbol table, scope, type, analyzer, and I/O tests
│   └── conformance/              # 66-program end-to-end conformance test suite
│       ├── valid/                # 25 valid programs covering all features
│       ├── invalid/              # 20 semantic (SEM-01..20) + 8 syntax error programs
│       ├── unsupported/          # 13 programs testing explicitly excluded features
│       └── test_conformance_runner.c # Automated conformance test harness
│
├── examples/
│   └── test.c                    # Specification conformance sample (Section 28.1)
│
├── docs/                         # Specifications and documentation
│   ├── architecture.md           # Architectural module breakdown
│   ├── semantic_rules.md         # Complete semantic type and diagnostic catalogue
│   └── testing.md                # Test execution guide and test suite breakdown
│
├── build/                        # Object files and test binaries
├── Makefile                      # GNU Make build script (warning-free with -Wall -Wextra -std=c99)
├── README.md                     # Project overview and usage
├── LICENSE                       # MIT License
└── .gitignore                    # Git artifact exclusions
```

---

## 5. Build and Execution Instructions

### Prerequisites
- GCC / MinGW C Compiler with C99 support
- GNU Make (`make` or `mingw32-make`)
- Windows / Linux / macOS compatible shell

### Building the Compiler
```bash
make clean
make
```
Produces `compiler.exe` (or `compiler` on Unix).

### Running the Compiler
```bash
# Basic invocation
./compiler.exe examples/test.c

# Verbose mode with raw token stream display
./compiler.exe --tokens examples/test.c
```

### Expected Output for `examples/test.c`
```text
========================================
        C Compiler Front-End
  Frozen Specification Version 1.0
========================================

Source file: examples/test.c
--- Beginning Lexical Analysis ---
--- Lexical Analysis Complete ---
Total tokens: 78, Lexical errors: 0

--- Beginning Syntax Analysis ---
--- Syntax Analysis Complete: Valid AST Constructed ---

--- Beginning Semantic Analysis ---
--- Semantic Analysis Complete: 0 Errors Found ---

========================================
Compilation Successful: examples/test.c validated.
========================================
```

---

## 6. Testing

Run all unit, integration, and conformance test suites:

```bash
# Complete Conformance Test Suite (66 end-to-end programs)
make test_conformance

# Regression Suites
make test_ast
make test_expressions
make test_statements
make test_program
make test_symbol_table
make test_scope_analysis
make test_type_system
make test_semantic_analyzer
make test_full_semantic
make test_io_semantics
```
All tests compile and run with 100% pass rates.
