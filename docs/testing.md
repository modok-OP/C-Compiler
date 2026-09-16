# Testing Guide and Verification Reference

This document describes the test strategy, test suites, execution commands, and verification results for the C Compiler front end according to the frozen **C Compiler Front-End Language & Semantic Specification (Version 1.0)**.

---

## 1. Test Suite Overview

The compiler front end is verified by a multi-tiered test infrastructure:

1. **Unit Test Suites**: Isolated verification of individual compiler modules (Lexer, AST, Symbol Table, Type System).
2. **Grammar & Parser Suites**: Expression precedence, statements, declarations, and function definitions.
3. **Semantic Analysis Suites**: Scope resolution, diagnostic error catalog validation (`SEM-01` through `SEM-20`), and I/O checking.
4. **End-to-End Pipeline Suites**: Integration tests validating the continuous flow from `.c` source text to AST and symbol table construction.
5. **Conformance Test Suite**: 66 complete `.c` programs categorized into valid programs, invalid semantic programs, invalid syntax programs, and unsupported feature programs.

---

## 2. Test Execution Commands

All tests are integrated into `Makefile` and can be built and executed with standard GNU Make:

```bash
# Build the main compiler
make clean
make

# Run the complete 66-program Conformance Suite
make test_conformance

# Run all individual unit and integration test targets
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

---

## 3. Conformance Test Suite Structure (`tests/conformance/`)

### 3.1 Valid Programs (`tests/conformance/valid/`) — 25 Tests
Every valid test program must successfully pass all three front-end stages (Lexical, Syntax, Semantic) and exit with code 0:
- `valid_01_basic_main.c`: Minimal valid entry point (`int main() { return 0; }`).
- `valid_02_global_vars.c`: Global variable declarations with `int`, `float`, and `char`.
- `valid_03_initialized_vars.c`: Global and local variable initializers.
- `valid_04_array_1d.c`: One-dimensional array declaration, indexing, and modification.
- `valid_05_function.c`: Single user-defined function.
- `valid_06_multiple_functions.c`: Multiple functions calling each other.
- `valid_07_function_parameters.c`: Multi-parameter function calls with diverse types.
- `valid_08_implicit_conversion.c`: Widening conversion (`int` $\rightarrow$ `float`) in assignment and expressions.
- `valid_09_explicit_casts.c`: Explicit type casts among `int`, `float`, and `char`.
- `valid_10_arithmetic.c`: Additive, multiplicative, modulus, increment, and decrement operations.
- `valid_11_comparisons.c`: Relational (`<`, `<=`, `>`, `>=`) and equality (`==`, `!=`) operators.
- `valid_12_logical.c`: Logical AND (`&&`), OR (`||`), and NOT (`!`) operations on booleans.
- `valid_13_if_else.c`: Nested `if`, `else if`, and `else` branches.
- `valid_14_while.c`: While loop with condition expression.
- `valid_15_do_while.c`: Do-while loop execution.
- `valid_16_for.c`: For-loop with initialization, condition, and update expressions.
- `valid_17_break.c`: Break statement within a loop.
- `valid_18_continue.c`: Continue statement within a loop.
- `valid_19_nested_blocks.c`: Multiple levels of nested `{ ... }` block scopes.
- `valid_20_shadowing.c`: Inner variable legally shadowing an outer declaration.
- `valid_21_function_calls.c`: Nested function call expressions.
- `valid_22_printf.c`: Printf with `%d`, `%f`, `%c`, `%s`, and combinations.
- `valid_23_scanf.c`: Scanf with `%d`, `%f`, `%c` and `&identifier` destinations.
- `valid_24_integration_mixed.c`: Full feature integration (functions, arrays, loops, I/O).
- `valid_25_official_spec.c`: The official Section 28.1 conformance program (`examples/test.c`).

### 3.2 Invalid Semantic Programs (`tests/conformance/invalid/`) — 20 Tests
Every program corresponds to an error code from the frozen error catalogue:
- `invalid_sem01_undeclared.c`: Reference to undeclared identifier (`SEM-01`).
- `invalid_sem02_duplicate_decl.c`: Duplicate variable in same scope (`SEM-02`).
- `invalid_sem03_out_of_scope.c`: Use of variable after its scope ended (`SEM-03`).
- `invalid_sem04_assignment_mismatch.c`: Assigning float to int without cast (`SEM-04`).
- `invalid_sem05_invalid_arithmetic.c`: Incompatible operand (`char + int`) (`SEM-05`).
- `invalid_sem06_invalid_modulus.c`: Modulus operator on float (`SEM-06`).
- `invalid_sem07_invalid_condition.c`: Non-boolean condition in `if` (`SEM-07`).
- `invalid_sem08_invalid_logical.c`: Non-boolean operands to `&&` (`SEM-08`).
- `invalid_sem09_wrong_arity.c`: Function call with too few arguments (`SEM-09`).
- `invalid_sem10_wrong_arg_type.c`: Function call with wrong argument type (`SEM-10`).
- `invalid_sem11_invalid_return_type.c`: Function return expression type mismatch (`SEM-11`).
- `invalid_sem12_missing_return.c`: Non-void function missing return statement (`SEM-12`).
- `invalid_sem13_break_outside_loop.c`: Break outside loop context (`SEM-13`).
- `invalid_sem14_continue_outside_loop.c`: Continue outside loop context (`SEM-14`).
- `invalid_sem15_invalid_array_index.c`: Array index expression is float (`SEM-15`).
- `invalid_sem16_array_assign_mismatch.c`: Assigning float to int array element (`SEM-16`).
- `invalid_sem17_printf_mismatch.c`: Printf format specifier / argument mismatch (`SEM-17`).
- `invalid_sem18_scanf_mismatch.c`: Scanf destination type mismatch (`SEM-18`).
- `invalid_sem19_invalid_cast.c`: Explicit cast from `void` (`SEM-19`).
- `invalid_sem20_invalid_main.c`: Invalid main return type (`void main()`) (`SEM-20`).

### 3.3 Invalid Syntax Programs (`tests/conformance/invalid/`) — 8 Tests
Verifies that syntax errors halt execution immediately and semantic analysis is never attempted:
- `invalid_syn01_missing_semicolon.c`: Semicolon omitted after declaration.
- `invalid_syn02_missing_brace.c`: Missing closing brace `}`.
- `invalid_syn03_unmatched_paren.c`: Unmatched opening parenthesis `(`.
- `invalid_syn04_malformed_expr.c`: Malformed expression (`10 + * 5`).
- `invalid_syn05_malformed_decl.c`: Missing identifier in declaration (`int = 10;`).
- `invalid_syn06_malformed_array_decl.c`: Missing array dimension (`int arr[;`).
- `invalid_syn07_malformed_func_decl.c`: Missing parameter name in function definition.
- `invalid_syn08_malformed_for.c`: Malformed for-loop header.

### 3.4 Unsupported Feature Programs (`tests/conformance/unsupported/`) — 13 Tests
Verifies that features excluded by the frozen specification are rejected cleanly without silent misinterpretation:
- `unsupported_01_pointer_declaration.c`: Pointer declaration `int *p;`.
- `unsupported_02_address_of_pointer.c`: Address-of `&` in assignment (`y = &x;`).
- `unsupported_03_dereference.c`: Pointer dereference `*x`.
- `unsupported_04_struct.c`: Struct declaration (`struct Point { ... };`).
- `unsupported_05_union.c`: Union declaration (`union Data { ... };`).
- `unsupported_06_enum.c`: Enum declaration (`enum Color { ... };`).
- `unsupported_07_typedef.c`: Typedef declaration (`typedef int Number;`).
- `unsupported_08_function_pointer.c`: Function pointer declaration (`int (*fp)(int);`).
- `unsupported_09_dynamic_memory.c`: Dynamic memory allocation call (`malloc(100);`).
- `unsupported_10_preprocessor.c`: Preprocessor directive (`#include <stdio.h>`).
- `unsupported_11_multidim_array.c`: Multi-dimensional array declaration (`int matrix[3][3];`).
- `unsupported_12_hex_literal.c`: Hexadecimal literal (`0x1A`).
- `unsupported_13_scientific_float.c`: Scientific notation literal (`1.5e10`).

---

## 4. Test Summary Results

| Test Target | Suite Description | Tests Passed | Pass Rate |
| :--- | :--- | :---: | :---: |
| `make test_ast` | AST Node Allocation & Deallocation | 1 / 1 | 100% |
| `make test_expressions` | Expression Precedence & Associativity | 49 / 49 | 100% |
| `make test_statements` | Declarations & Control Statements | 42 / 42 | 100% |
| `make test_program` | Functions & Program-Level Parsing | 14 / 14 | 100% |
| `make test_symbol_table` | Symbol Table Operations & Lookups | 11 / 11 | 100% |
| `make test_scope_analysis` | Scoped Symbol Table Population | 5 / 5 | 100% |
| `make test_type_system` | Semantic Type System & Cast Matrix | 22 / 22 | 100% |
| `make test_semantic_analyzer` | Unit Semantic Checks (SEM-01..20) | 29 / 29 | 100% |
| `make test_full_semantic` | Pipeline Feature Integration | 2 / 2 | 100% |
| `make test_io_semantics` | Printf / Scanf / Ampersand Suite | 33 / 33 | 100% |
| `make test_conformance` | End-to-End Conformance Test Suite | 66 / 66 | 100% |
