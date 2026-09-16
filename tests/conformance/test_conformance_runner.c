#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbol_table.h"
#include "semantic.h"

typedef struct {
    int lex_errors;
    int syn_errors;
    int sem_errors;
    int total_tokens;
} PipelineResult;

static PipelineResult run_pipeline(const char *filepath) {
    PipelineResult res = {0, 0, 0, 0};

    /* 1. Lexical pass */
    Lexer *lexer = lexer_create_from_file(filepath);
    if (!lexer) {
        res.lex_errors = 1;
        return res;
    }

    Token tok;
    do {
        tok = lexer_next_token(lexer);
        res.total_tokens++;
        if (tok.type == TOKEN_LEXICAL_ERROR) {
            res.lex_errors++;
        }
        token_free(&tok);
    } while (tok.type != TOKEN_EOF);
    lexer_destroy(lexer);

    if (res.lex_errors > 0) {
        return res;
    }

    /* 2. Syntax pass */
    Lexer *parse_lexer = lexer_create_from_file(filepath);
    if (!parse_lexer) {
        res.syn_errors = 1;
        return res;
    }

    Parser *parser = parser_create(parse_lexer);
    ASTNode *ast = parser_parse_program(parser);
    res.syn_errors = parser->error_count;

    if (!ast || res.syn_errors > 0) {
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(parse_lexer);
        return res;
    }

    /* 3. Semantic pass */
    SemanticContext *ctx = semantic_context_create();
    res.sem_errors = semantic_analyze(ctx, ast);

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(parse_lexer);

    return res;
}

static int total_conformance_tests = 0;
static int passed_conformance_tests = 0;

static void test_valid_file(const char *filepath, const char *desc) {
    total_conformance_tests++;
    PipelineResult res = run_pipeline(filepath);

    int ok = (res.lex_errors == 0 && res.syn_errors == 0 && res.sem_errors == 0);
    if (ok) {
        printf("[PASS] Valid: %s (%s)\n", filepath, desc);
        passed_conformance_tests++;
    } else {
        printf("[FAIL] Valid: %s (%s) -> Lex:%d Syn:%d Sem:%d\n",
               filepath, desc, res.lex_errors, res.syn_errors, res.sem_errors);
    }
}

static void test_invalid_semantic_file(const char *filepath, const char *expected_code, const char *desc) {
    total_conformance_tests++;
    PipelineResult res = run_pipeline(filepath);

    int ok = (res.lex_errors == 0 && res.syn_errors == 0 && res.sem_errors > 0);
    if (ok) {
        printf("[PASS] Invalid Semantic: %s [%s] (%s) -> Caught %d error(s)\n",
               filepath, expected_code, desc, res.sem_errors);
        passed_conformance_tests++;
    } else {
        printf("[FAIL] Invalid Semantic: %s [%s] (%s) -> Lex:%d Syn:%d Sem:%d\n",
               filepath, expected_code, desc, res.lex_errors, res.syn_errors, res.sem_errors);
    }
}

static void test_invalid_syntax_file(const char *filepath, const char *desc) {
    total_conformance_tests++;
    PipelineResult res = run_pipeline(filepath);

    /* Must fail at syntax stage; semantic stage must not run */
    int ok = (res.lex_errors == 0 && res.syn_errors > 0 && res.sem_errors == 0);
    if (ok) {
        printf("[PASS] Invalid Syntax: %s (%s) -> Caught %d syntax error(s)\n",
               filepath, desc, res.syn_errors);
        passed_conformance_tests++;
    } else {
        printf("[FAIL] Invalid Syntax: %s (%s) -> Lex:%d Syn:%d Sem:%d\n",
               filepath, desc, res.lex_errors, res.syn_errors, res.sem_errors);
    }
}

static void test_unsupported_file(const char *filepath, const char *desc) {
    total_conformance_tests++;
    PipelineResult res = run_pipeline(filepath);

    /* Must be rejected at any stage (lex, syn, or sem) cleanly without crashing */
    int total_errors = res.lex_errors + res.syn_errors + res.sem_errors;
    if (total_errors > 0) {
        printf("[PASS] Unsupported: %s (%s) -> Rejected cleanly with %d error(s)\n",
               filepath, desc, total_errors);
        passed_conformance_tests++;
    } else {
        printf("[FAIL] Unsupported: %s (%s) -> Was incorrectly accepted as valid!\n",
               filepath, desc);
    }
}

int main(void) {
    printf("====================================================\n");
    printf("     C COMPILER FRONT-END CONFORMANCE TEST SUITE     \n");
    printf("           Frozen Specification Version 1.0         \n");
    printf("====================================================\n\n");

    /* ========================================================
     * 1. Valid Conformance Programs (25 tests)
     * ======================================================== */
    printf("--- 1. Testing Valid Programs (Expected: 0 errors) ---\n");
    test_valid_file("tests/conformance/valid/valid_01_basic_main.c", "Basic main function");
    test_valid_file("tests/conformance/valid/valid_02_global_vars.c", "Global scalar variables");
    test_valid_file("tests/conformance/valid/valid_03_initialized_vars.c", "Initialized variables");
    test_valid_file("tests/conformance/valid/valid_04_array_1d.c", "1-D array declaration and access");
    test_valid_file("tests/conformance/valid/valid_05_function.c", "Single function call");
    test_valid_file("tests/conformance/valid/valid_06_multiple_functions.c", "Multiple functions");
    test_valid_file("tests/conformance/valid/valid_07_function_parameters.c", "Function parameter passing");
    test_valid_file("tests/conformance/valid/valid_08_implicit_conversion.c", "Implicit int to float widening");
    test_valid_file("tests/conformance/valid/valid_09_explicit_casts.c", "Explicit type casts");
    test_valid_file("tests/conformance/valid/valid_10_arithmetic.c", "Arithmetic operators");
    test_valid_file("tests/conformance/valid/valid_11_comparisons.c", "Comparison operators");
    test_valid_file("tests/conformance/valid/valid_12_logical.c", "Logical operators");
    test_valid_file("tests/conformance/valid/valid_13_if_else.c", "If-else control flow");
    test_valid_file("tests/conformance/valid/valid_14_while.c", "While loop");
    test_valid_file("tests/conformance/valid/valid_15_do_while.c", "Do-while loop");
    test_valid_file("tests/conformance/valid/valid_16_for.c", "For loop");
    test_valid_file("tests/conformance/valid/valid_17_break.c", "Break statement");
    test_valid_file("tests/conformance/valid/valid_18_continue.c", "Continue statement");
    test_valid_file("tests/conformance/valid/valid_19_nested_blocks.c", "Nested block scopes");
    test_valid_file("tests/conformance/valid/valid_20_shadowing.c", "Identifier shadowing");
    test_valid_file("tests/conformance/valid/valid_21_function_calls.c", "Function call expressions");
    test_valid_file("tests/conformance/valid/valid_22_printf.c", "Printf with all specifiers");
    test_valid_file("tests/conformance/valid/valid_23_scanf.c", "Scanf with & destinations");
    test_valid_file("tests/conformance/valid/valid_24_integration_mixed.c", "Mixed feature integration");
    test_valid_file("tests/conformance/valid/valid_25_official_spec.c", "Official Section 28.1 example");

    /* ========================================================
     * 2. Invalid Semantic Programs (20 tests: SEM-01 ... SEM-20)
     * ======================================================== */
    printf("\n--- 2. Testing Invalid Semantic Programs (Expected: Semantic Error) ---\n");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem01_undeclared.c", "SEM-01", "Undeclared identifier");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem02_duplicate_decl.c", "SEM-02", "Duplicate declaration in same scope");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem03_out_of_scope.c", "SEM-03", "Out-of-scope identifier");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem04_assignment_mismatch.c", "SEM-04", "Assignment type mismatch (float to int)");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem05_invalid_arithmetic.c", "SEM-05", "Invalid arithmetic operand (char + int)");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem06_invalid_modulus.c", "SEM-06", "Invalid modulus operand (float % int)");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem07_invalid_condition.c", "SEM-07", "Non-boolean condition in if");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem08_invalid_logical.c", "SEM-08", "Non-boolean operands to logical &&");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem09_wrong_arity.c", "SEM-09", "Wrong number of arguments");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem10_wrong_arg_type.c", "SEM-10", "Wrong argument type");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem11_invalid_return_type.c", "SEM-11", "Return type mismatch");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem12_missing_return.c", "SEM-12", "Missing return in non-void function");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem13_break_outside_loop.c", "SEM-13", "Break outside loop");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem14_continue_outside_loop.c", "SEM-14", "Continue outside loop");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem15_invalid_array_index.c", "SEM-15", "Non-integer array index");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem16_array_assign_mismatch.c", "SEM-16", "Array element assignment mismatch");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem17_printf_mismatch.c", "SEM-17", "Printf format type mismatch");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem18_scanf_mismatch.c", "SEM-18", "Scanf destination type mismatch");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem19_invalid_cast.c", "SEM-19", "Unsupported cast from void");
    test_invalid_semantic_file("tests/conformance/invalid/invalid_sem20_invalid_main.c", "SEM-20", "Invalid main function return type");

    /* ========================================================
     * 3. Invalid Syntax Programs (8 tests)
     * ======================================================== */
    printf("\n--- 3. Testing Invalid Syntax Programs (Expected: Syntax Error, Semantic Skipped) ---\n");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn01_missing_semicolon.c", "Missing semicolon");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn02_missing_brace.c", "Missing closing brace");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn03_unmatched_paren.c", "Unmatched opening parenthesis");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn04_malformed_expr.c", "Malformed binary expression");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn05_malformed_decl.c", "Missing identifier in declaration");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn06_malformed_array_decl.c", "Missing array dimension");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn07_malformed_func_decl.c", "Missing parameter name in function");
    test_invalid_syntax_file("tests/conformance/invalid/invalid_syn08_malformed_for.c", "Malformed for-loop header");

    /* ========================================================
     * 4. Unsupported Feature Programs (13 tests)
     * ======================================================== */
    printf("\n--- 4. Testing Unsupported Features (Expected: Clean Rejection) ---\n");
    test_unsupported_file("tests/conformance/unsupported/unsupported_01_pointer_declaration.c", "Pointer declaration 'int *p'");
    test_unsupported_file("tests/conformance/unsupported/unsupported_02_address_of_pointer.c", "Address-of operator '&' in assignment");
    test_unsupported_file("tests/conformance/unsupported/unsupported_03_dereference.c", "Pointer dereference '*x'");
    test_unsupported_file("tests/conformance/unsupported/unsupported_04_struct.c", "Struct declaration");
    test_unsupported_file("tests/conformance/unsupported/unsupported_05_union.c", "Union declaration");
    test_unsupported_file("tests/conformance/unsupported/unsupported_06_enum.c", "Enum declaration");
    test_unsupported_file("tests/conformance/unsupported/unsupported_07_typedef.c", "Typedef declaration");
    test_unsupported_file("tests/conformance/unsupported/unsupported_08_function_pointer.c", "Function pointer");
    test_unsupported_file("tests/conformance/unsupported/unsupported_09_dynamic_memory.c", "Dynamic memory allocation malloc");
    test_unsupported_file("tests/conformance/unsupported/unsupported_10_preprocessor.c", "Preprocessor directive #include");
    test_unsupported_file("tests/conformance/unsupported/unsupported_11_multidim_array.c", "Multi-dimensional array");
    test_unsupported_file("tests/conformance/unsupported/unsupported_12_hex_literal.c", "Hexadecimal literal 0x1A");
    test_unsupported_file("tests/conformance/unsupported/unsupported_13_scientific_float.c", "Scientific notation float 1.5e10");

    printf("\n====================================================\n");
    printf("CONFORMANCE SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_conformance_tests, total_conformance_tests,
           (float)passed_conformance_tests / total_conformance_tests * 100.0f);
    printf("====================================================\n");

    return (passed_conformance_tests == total_conformance_tests) ? 0 : 1;
}
