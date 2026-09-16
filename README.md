# C Compiler Front-End

## Project Name
**C Compiler Front-End (Frozen Specification v1.0)**

## Project Purpose
The purpose of this project is to design and implement a robust compiler front end for a defined subset of the C programming language. The front end takes C source code (.c), performs lexical analysis, syntax analysis (parsing into an Abstract Syntax Tree), constructs a scoped symbol table, and carries out semantic analysis and type checking.

> **Important**: The implementation will follow the frozen language specification.

No language features will be invented, changed, simplified, or added beyond what is defined in C_Compiler_Front_End_Language_Specification_v1.0.docx.

---

## Technical Specifications
- **Implementation Language**: C (compiled with GCC/MinGW)
- **Input Language**: C (defined project subset)
- **Input File Extension**: .c
- **Current Project Status**: Stage 1 — Project Foundation (Project structure, build system, baseline driver, documentation, and Git repository initialized).

---

## Planned Compiler Architecture

The compiler front end operates as a multi-stage pipeline:

`
C Source File (.c)
       │
       ▼
Lexical Analyzer (Lexer)
       │
       ▼
  Token Stream
       │
       ▼
Syntax Analyzer (Recursive Descent Parser)
       │
       ▼
Abstract Syntax Tree (AST)
       │
       ▼
Semantic Analyzer & Symbol Table
       │
       ▼
Validated AST & Diagnostics (Front-End Completion)
       │
       ▼
[Future Stages: IR Generation, Optimization, Target Code Generation]
`

Each stage performs a distinct role:
1. **Lexer**: Scans input source characters into structured tokens with location metadata (line and column) and detects lexical anomalies.
2. **Parser**: Recursively parses tokens against the LL(1) grammar, verifying syntax and generating an AST.
3. **AST**: Provides a clean intermediate tree structure reflecting syntactic and semantic hierarchies.
4. **Symbol Table**: Manages nested lexical scopes, type metadata, identifiers, and predefined function signatures (printf, scanf).
5. **Semantic Analyzer**: Enforces type rules, conversion constraints, function signatures, loop context, array indexing, and format specifier matching.

---

## Repository Structure

`
C-Compiler/
│
├── src/                  # Source implementation files (.c)
│   └── main.c            # Compiler CLI driver entry point
├── include/              # Header files (.h) exposing module interfaces
├── tests/                # Verification and test suites
│   ├── lexer/            # Lexer test inputs and expected outputs
│   ├── parser/           # Parser syntax test cases
│   └── semantic/         # Semantic analysis test cases (valid and SEM-01..SEM-20)
├── examples/             # Sample C-subset programs (.c)
├── docs/                 # Documentation and architectural specifications
│   └── architecture.md   # Architectural design document
├── build/                # Build artifacts and compiled objects
├── .gitignore            # Git exclusion rules for Windows/MinGW
├── README.md             # Project documentation (this file)
└── Makefile              # Build automation for GCC/MinGW
`

---

## Build System

The project uses GNU Make (make or mingw32-make) with gcc:

- **Build Compiler**:
  `ash
  make
  `
  Produces compiler.exe in the root directory.

- **Clean Build Artifacts**:
  `ash
  make clean
  `
  Removes compiled object files (uild/*.o) and the output executable (compiler.exe).

---

## Development Stages

Development follows an incremental roadmap defined in the specification:

1. **Stage 1 — Project Skeleton & Foundation** *(Current)*: Project layout, build automation, architecture documentation, CLI driver stub.
2. **Stage 2 — Token Definitions & Lexer**: Token kinds, character scanning, location tracking, and lexical diagnostics.
3. **Stage 3 — Abstract Syntax Tree (AST)**: AST node types, tree constructors, and AST visualizer/dumper.
4. **Stage 4 — Parser (Expressions)**: Operator precedence, associativity, and expression tree construction.
5. **Stage 5 — Parser (Statements & Declarations)**: Control structures, functions, arrays, and syntax error diagnosis.
6. **Stage 6 — Symbol Table**: Scoped identifier tracking, shadowing policy, function signatures, and predefined I/O.
7. **Stage 7 — Semantic Analyzer**: Full type checking, semantic validation, and diagnostic error catalogue (SEM-01 to SEM-20).
8. **Stage 8 — Integration**: End-to-end front-end pipeline execution from .c file to semantic validation.
9. **Stage 9 — Regression Testing**: Comprehensive test suite verification across valid programs and invalid error cases.
