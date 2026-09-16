# Compiler Architecture Specification

This document details the front-end architecture for the C Compiler project based on the frozen **C Compiler Front-End Language & Semantic Specification (Version 1.0)**.

The scope of this implementation encompasses front-end compilation through semantic validation. Subsequent phases (Intermediate Representation, Optimization, and Target Code Generation) will consume the validated Abstract Syntax Tree (AST) and Symbol Table.

---

## 1. Compiler Front-End Pipeline

The front-end pipeline follows a classical decoupled, multi-pass staged architecture:

```
+-------------------------------------------------------------+
|                     C Source File (.c)                      |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|               Lexical Analyzer (src/lexer.c)                |
|  - Character scanning, whitespace/comment stripping         |
|  - Token categorization, keyword recognition                |
|  - Line and column source location tracking                 |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                 Token Stream (src/token.c)                  |
|  - Tokens: kind, lexeme, line, column                       |
|  - Reusable, dynamically deallocated token representation    |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|           Syntax Analyzer / Parser (src/parser.c)           |
|  - Predictive recursive descent parsing                     |
|  - 3-token lookahead buffer (current, next, next2)          |
|  - Operator precedence climbing and unambiguous expressions |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|             Abstract Syntax Tree (src/ast.c)                |
|  - Clean structural hierarchy preserving semantic nodes     |
|  - 18 node categories with explicit typed unions            |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|             Symbol Table (src/symbol_table.c)               |
|  - Lexically scoped symbol environments (global/func/block) |
|  - Predefined function signatures (printf, scanf)           |
|  - Inner-block shadowing and lookup chaining                |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|          Semantic Type System (src/type_system.c)           |
|  - Primitive types (INT, FLOAT, CHAR, VOID, BOOL, STRING)   |
|  - Implicit widening (int -> float only)                    |
|  - Explicit cast matrix and operator compatibility rules    |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|            Semantic Analyzer (src/semantic.c)               |
|  - AST traversal and recursive expression type inference    |
|  - Name resolution & out-of-scope distinction (SEM-01, 03)  |
|  - Declarations, assignments, and arrays (SEM-02, 04, 15,16)|
|  - Control flow & loop context (SEM-07, 08, 13, 14)         |
|  - Function signatures, return completeness (SEM-09..12, 20)|
|  - Printf / scanf format checking & & restriction (SEM-17,18)|
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|             Front-End Result / Validated AST                |
|  - 0 Errors -> Compilation Successful                       |
|  - Errors   -> Source-located diagnostics (Line:Col)        |
+-------------------------------------------------------------+
```

---

## 2. Detailed Architectural Modules

### 2.1 Driver (`src/main.c`)
- **Responsibility**: Orchestrates the multi-pass compilation process from `.c` source input to diagnostic summary.
- **Stage Isolation**: Strictly halts execution if an earlier phase encounters errors:
  1. If lexical errors exist, halts before syntax analysis.
  2. If syntax errors exist or AST generation fails, halts before semantic analysis.
  3. If semantic errors exist, reports diagnostics and exits with code 1; does not report compilation success.
- **CLI Interface**: Supports standard clean output and optional `--tokens` / `-v` flag for detailed token streams.

### 2.2 Token System (`include/token.h`, `src/token.c`)
- **Responsibility**: Defines all 49 token categories required by the frozen specification.
- **Categories**:
  - Keywords: `int`, `float`, `char`, `void`, `main`, `if`, `else`, `while`, `for`, `do`, `return`, `break`, `continue`.
  - Identifiers: `[a-zA-Z_][a-zA-Z0-9_]*`.
  - Literals: `TOKEN_INT_LITERAL`, `TOKEN_FLOAT_LITERAL`, `TOKEN_CHAR_LITERAL`, `TOKEN_STRING_LITERAL`.
  - Operators: Arithmetic, relational, equality, logical, assignment, compound assignment, increment/decrement, and ampersand.
  - Delimiters: `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,`.
  - Special: `TOKEN_EOF`, `TOKEN_LEXICAL_ERROR`.
- **Memory Management**: Every token owns a duplicated lexeme string, freed via `token_free()`.

### 2.3 Lexical Analyzer (`include/lexer.h`, `src/lexer.c`)
- **Responsibility**: Character-level scanning of source files or in-memory strings.
- **Features**:
  - Automatically skips whitespace and tracks 1-indexed `line` and `column` coordinates.
  - Strips both single-line (`//`) and multi-line (`/* ... */`) comments.
  - Diagnoses unterminated multi-line comments, unterminated strings, and invalid characters as `TOKEN_LEXICAL_ERROR`.

### 2.4 Recursive Descent Parser (`include/parser.h`, `src/parser.c`)
- **Responsibility**: Verifies syntax and constructs the AST according to the frozen grammar.
- **Architecture**:
  - Predictive recursive-descent parser.
  - 3-token lookahead buffer (`current`, `next`, `next2`) to disambiguate grammar constructs (such as variable declarations vs. function definitions vs. statements).
  - Eliminates left recursion using iterative tail parsing for binary operators while maintaining correct left-associativity and precedence.
  - Gracefully recovers from syntax errors without crashing.

### 2.5 Abstract Syntax Tree (`include/ast.h`, `src/ast.c`)
- **Responsibility**: Provides an immutable, hierarchically structured in-memory representation of the program.
- **Categories**:
  - `AST_PROGRAM`: List of top-level declarations and function definitions.
  - `AST_FUNCTION`: Function definition with return type, parameter array, and body block.
  - `AST_BLOCK`: Ordered list of statements within `{ ... }`.
  - `AST_DECLARATION`: Scalar or 1-D array declaration with optional initializer.
  - `AST_ASSIGNMENT`: Simple and compound assignments.
  - `AST_IF`: If / else branching node.
  - `AST_WHILE`, `AST_DO_WHILE`, `AST_FOR`: Iteration loop structures.
  - `AST_RETURN`, `AST_BREAK`, `AST_CONTINUE`: Jump statements.
  - `AST_CALL`: Callee identifier and argument list.
  - `AST_ARRAY_ACCESS`: Array subscript expression.
  - `AST_BINARY_EXPR`, `AST_UNARY_EXPR`, `AST_CAST`: Arithmetic and logical operations.
  - `AST_LITERAL`, `AST_IDENTIFIER`: Terminal leaf nodes.
- **Clean Destruction**: `ast_free()` performs full recursive deallocation of all subtrees, parameter arrays, and duplicated strings.

### 2.6 Symbol Table & Scopes (`include/symbol_table.h`, `src/symbol_table.c`)
- **Responsibility**: Manages identifier bindings across lexical scopes.
- **Scope Hierarchy**:
  - Tree-structured scopes: `SCOPE_GLOBAL` $\rightarrow$ `SCOPE_FUNCTION` $\rightarrow$ `SCOPE_BLOCK`.
  - Supports lexical scoping with parent-link traversal.
  - Allows legal inner-block variable shadowing while forbidding duplicate declarations within the same scope (`SEM-02`).
  - Pre-populates the global environment with `printf` and `scanf` predefined function signatures.

### 2.7 Semantic Type System (`include/type_system.h`, `src/type_system.c`)
- **Responsibility**: Centralizes type classification, implicit conversions, cast matrix checks, and operator validity.
- **Types**: `SEM_TYPE_INT`, `SEM_TYPE_FLOAT`, `SEM_TYPE_CHAR`, `SEM_TYPE_VOID`, `SEM_TYPE_BOOL`, `SEM_TYPE_STRING`, `SEM_TYPE_ERROR`.
- **Rules**:
  - Widening: `int` $\rightarrow$ `float` is the only permissible implicit cross-type conversion.
  - Explicit casts: Supported between `int`, `float`, and `char`.
  - Strict boolean conditions: Relational and logical expressions produce `SEM_TYPE_BOOL`; arbitrary numeric truthiness is disallowed.

### 2.8 Semantic Analyzer (`include/semantic.h`, `src/semantic.c`)
- **Responsibility**: Traverses the AST and enforces all semantic rules defined in Sections 13–22 of the specification.
- **Error Detection**: Diagnoses and reports every error family from `SEM-01` to `SEM-20` with line and column accuracy.
- **I/O Format Checking**:
  - `printf`: Verifies format string is a string literal, extracts specifiers (`%d`, `%f`, `%c`, `%s`), matches argument counts, and enforces strict argument types.
  - `scanf`: Verifies format string is a string literal, extracts specifiers (`%d`, `%f`, `%c`), matches destination counts, and enforces `&identifier` destination syntax and type compatibility.
  - Restricts the standalone address-of operator `&` exclusively to `scanf` destinations.
