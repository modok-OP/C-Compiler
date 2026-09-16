# Compiler Architecture Specification

This document details the front-end architecture for the C Compiler project based on the frozen **C Compiler Front-End Language & Semantic Specification (Version 1.0)**.

The current project scope implements the compiler front end through semantic validation. Subsequent phases (Intermediate Representation, Optimization, and Target Code Generation) will consume the validated Abstract Syntax Tree (AST) and Symbol Table.

---

## 1. Compiler Front-End Pipeline

The front-end pipeline follows a classic staged architecture:

`
+---------------------------+
|    C Source File (.c)     |
+---------------------------+
              |
              v
+---------------------------+
|   Lexical Analyzer (Lexer)|
+---------------------------+
              |
              v
+---------------------------+
|        Token Stream       |
+---------------------------+
              |
              v
+---------------------------+
|  Syntax Analyzer (Parser) |
|    (Recursive Descent)    |
+---------------------------+
              |
              v
+---------------------------+
| Abstract Syntax Tree (AST)|
+---------------------------+
              |
              v
+---------------------------+
|     Semantic Analyzer     | <---> [ Symbol Table ]
+---------------------------+
              |
      +-------+-------+
      |               |
      v               v
Validated AST   Semantic Errors
      |
      v
+---------------------------+
|   Future Compiler Stages  |
|  (IR, Optimization, Code  |
|        Generation)        |
+---------------------------+
`

---

## 2. Architectural Modules Overview

### 2.1 Driver (src/main.c)
- **Primary Responsibility**: Entry point and pipeline orchestrator.
- **Input**: Command-line arguments specifying input source file (.c).
- **Output**: Controls pipeline flow, manages compiler flags, and reports overall compilation summary and diagnostic status.
- **High-Level Purpose**: Coordinates reading source code, invoking the lexer, parser, AST generation, and semantic analysis, then formatting diagnostic output for the user.

### 2.2 Lexical Analyzer / Lexer (src/lexer.c, include/lexer.h, include/token.h)
- **Primary Responsibility**: Character scanning, tokenization, and source location tracking.
- **Input**: Raw C source character stream (.c file).
- **Output**: Sequence of lexical tokens (token kind, lexeme string, line number, column number) or lexical diagnostics.
- **High-Level Purpose**:
  - Strips whitespace and comments (both // single-line and /* ... */ multi-line comments).
  - Recognizes keywords (int, loat, char, oid, main, if, else, while, or, do, eturn, reak, continue).
  - Identifies identifiers matching (letter | _) (letter | digit | _)*.
  - Scans integer literals, floating-point literals, character literals, and string literals.
  - Scans operators (+, -, *, /, %, <, >, <=, >=, ==, !=, &&, ||, !, =, +=, -=, *=, /=, %=, ++, --).
  - Scans delimiters ((, ), {, }, [, ], ;, ,) and special input symbol (& for scanf).
  - Flags lexical errors such as malformed numbers, invalid characters, unterminated strings, and unterminated multi-line comments.

### 2.3 Syntax Analyzer / Recursive Descent Parser (src/parser.c, include/parser.h)
- **Primary Responsibility**: Grammar validation and syntax tree construction.
- **Input**: Token stream from the lexer.
- **Output**: Abstract Syntax Tree (AST) representing the program structure, or syntax diagnostics.
- **High-Level Purpose**:
  - Implements predictive recursive-descent parsing based on the frozen LL(1)-compatible grammar specification.
  - Parses external declarations, function definitions (int main(), typed functions with parameters), variable/array declarations, blocks, statements (if/else, while, do-while, or, eturn, reak, continue, expressions), and expressions.
  - Enforces operator precedence and associativity across all expression tiers without left recursion.
  - Emits clear syntax error messages with line and column locations if source code violates grammar rules.

### 2.4 Abstract Syntax Tree (AST) (src/ast.c, include/ast.h)
- **Primary Responsibility**: Structural intermediate representation of the parsed program.
- **Input**: Structural nodes created during parsing.
- **Output**: Clean hierarchical tree structure preserving semantic intent without punctuation/grammar noise.
- **High-Level Purpose**:
  - Represents Program, Function, Block, Declaration, Assignment, If, While, DoWhile, For, Return, Break, Continue, Call, Array Access, Binary Expression, Unary Expression, Cast, Literal, and Identifier nodes.
  - Serves as the primary data structure for semantic validation and future code generation passes.

### 2.5 Symbol Table (src/symbol_table.c, include/symbol_table.h)
- **Primary Responsibility**: Scoped identifier storage, lookup, and environment management.
- **Input**: Identifier declarations and scope transitions.
- **Output**: Resolution of identifier declarations, types, and scope attributes.
- **High-Level Purpose**:
  - Manages hierarchical/nested scopes (global scope, function scope, block scope).
  - Stores symbol information: symbol name, kind (variable, array, function, parameter, predefined function), type (int, loat, char, oid), scope level, array dimensions, and function signatures.
  - Pre-populates built-in I/O signatures (printf, scanf).
  - Enforces scope visibility, inner-block shadowing policy, and detection of duplicate declarations in the same scope.

### 2.6 Semantic Analyzer (src/semantic.c, include/semantic.h)
- **Primary Responsibility**: Semantic validation, type checking, and consistency enforcement.
- **Input**: AST + Symbol Table.
- **Output**: Annotated/validated AST and diagnostic report cataloging semantic errors (SEM-01 through SEM-20).
- **High-Level Purpose**:
  - Verifies declaration-before-use and scope access rules.
  - Validates main() signature (must be parameterless int main()) and ensures exactly one entry point.
  - Type checking: assignment compatibility, arithmetic operands, boolean requirements for logical and conditional expressions.
  - Enforces implicit conversion restrictions (only widening int -> loat allowed implicitly; explicit casts required otherwise).
  - Checks control-flow constraints (reak and continue only valid inside loops).
  - Validates function call arity, parameter types, and return expression types against function signatures.
  - Enforces 1-D array indexing with integer types and element assignment compatibility.
  - Validates printf and scanf format specifiers against argument types and destination addresses (&identifier).

### 2.7 Future Compiler Stages (Beyond Current Scope)
- **Intermediate Representation (IR)**: Three-address code or quadruples generated from validated AST.
- **Code Optimization**: Control-flow analysis, dead code elimination, constant folding, and register allocation.
- **Target Code Generation**: Machine code or assembly emission for target architectures (e.g., x86/x86-64).

---

## 3. Implementation Guidelines
- Modular C architecture: all modules expose interfaces via header files under include/ and implementations under src/.
- Strict conformance to the frozen language specification without inventing, altering, or omitting features.
- Incremental development: each module will be built, tested, and validated step-by-step with regression test suites.
