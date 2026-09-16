#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbol_table.h"
#include "type_system.h"
#include "semantic.h"

static int total_tests = 0;
static int passed_tests = 0;

static void test_valid_io(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Valid I/O Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<valid_io_test>");
    if (!lexer) {
        printf("FAILED: Lexer initialization failed\n\n");
        return;
    }

    Parser *parser = parser_create(lexer);
    if (!parser) {
        printf("FAILED: Parser initialization failed\n\n");
        lexer_destroy(lexer);
        return;
    }

    ASTNode *ast = parser_parse_program(parser);
    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing encountered %d error(s)\n\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int sem_errors = semantic_analyze(ctx, ast);

    if (sem_errors == 0) {
        printf("RESULT: PASSED (0 semantic errors)\n");
        passed_tests++;
    } else {
        printf("RESULT: FAILED (%d semantic error(s) encountered)\n", sem_errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_invalid_io(const char *src, const char *desc, const char *expected_sem_id) {
    total_tests++;
    printf("====================================================\n");
    printf("Invalid I/O Test %02d [%s]: Expected [%s]\n%s\n", total_tests, desc, expected_sem_id, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<invalid_io_test>");
    if (!lexer) {
        printf("FAILED: Lexer initialization failed\n\n");
        return;
    }

    Parser *parser = parser_create(lexer);
    if (!parser) {
        printf("FAILED: Parser initialization failed\n\n");
        lexer_destroy(lexer);
        return;
    }

    ASTNode *ast = parser_parse_program(parser);
    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing encountered %d error(s)\n\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int sem_errors = semantic_analyze(ctx, ast);

    if (sem_errors > 0) {
        printf("RESULT: PASSED (Caught %d semantic error(s) as expected for %s)\n", sem_errors, expected_sem_id);
        passed_tests++;
    } else {
        printf("RESULT: FAILED (Expected %s semantic error, but analysis succeeded!)\n", expected_sem_id);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_multi_error_recovery(const char *src, const char *desc, int min_expected_errors) {
    total_tests++;
    printf("====================================================\n");
    printf("Multi-Error Recovery Test %02d [%s]: Expected >= %d errors\n%s\n", total_tests, desc, min_expected_errors, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<multi_error_test>");
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing failed\n\n");
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int sem_errors = semantic_analyze(ctx, ast);

    if (sem_errors >= min_expected_errors) {
        printf("RESULT: PASSED (Recovered and reported %d semantic errors without compiler crash)\n", sem_errors);
        passed_tests++;
    } else {
        printf("RESULT: FAILED (Expected at least %d errors, got %d)\n", min_expected_errors, sem_errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

int main(void) {
    printf("####################################################\n");
    printf("#  STAGE 10: PRINTF / SCANF & I/O SEMANTIC SUITE   #\n");
    printf("####################################################\n\n");

    /* ========================================================
     * TASK 13: Valid printf calls
     * ======================================================== */
    test_valid_io(
        "int main() {\n"
        "    printf(\"Hello, World!\\n\");\n"
        "    return 0;\n"
        "}\n",
        "printf literal string without format specifiers"
    );

    test_valid_io(
        "int main() {\n"
        "    int x = 42;\n"
        "    printf(\"%d\\n\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf with %d and int variable"
    );

    test_valid_io(
        "int main() {\n"
        "    float f = 3.14;\n"
        "    printf(\"%f\\n\", f);\n"
        "    return 0;\n"
        "}\n",
        "printf with %f and float variable"
    );

    test_valid_io(
        "int main() {\n"
        "    char c = 'A';\n"
        "    printf(\"%c\\n\", c);\n"
        "    return 0;\n"
        "}\n",
        "printf with %c and char variable"
    );

    test_valid_io(
        "int main() {\n"
        "    printf(\"%s\\n\", \"hello\");\n"
        "    return 0;\n"
        "}\n",
        "printf with %s and string literal"
    );

    test_valid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    float f = 20.5;\n"
        "    char c = 'Z';\n"
        "    printf(\"%d %f %c\\n\", x, f, c);\n"
        "    return 0;\n"
        "}\n",
        "printf with multiple specifiers (%d %f %c)"
    );

    test_valid_io(
        "int main() {\n"
        "    int x = 100;\n"
        "    float f = 200.5;\n"
        "    printf(\"x=%d y=%f\\n\", x, f);\n"
        "    return 0;\n"
        "}\n",
        "printf with interspersed text and specifiers"
    );

    /* ========================================================
     * TASK 14: Invalid printf calls (SEM-17)
     * ======================================================== */
    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    printf(x);\n"
        "    return 0;\n"
        "}\n",
        "printf first argument is variable instead of string literal",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    float f = 3.14;\n"
        "    printf(\"%d\", f);\n"
        "    return 0;\n"
        "}\n",
        "printf %d with float argument mismatch",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    printf(\"%f\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf %f with int argument mismatch",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 65;\n"
        "    printf(\"%c\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf %c with int argument mismatch",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    printf(\"%s\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf %s with non-string literal argument",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    printf(\"%d %d\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf format requires 2 arguments, got 1",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    int y = 20;\n"
        "    printf(\"%d\", x, y);\n"
        "    return 0;\n"
        "}\n",
        "printf format requires 1 argument, got 2",
        "SEM-17"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    printf(\"%x\", x);\n"
        "    return 0;\n"
        "}\n",
        "printf unsupported format specifier %x",
        "SEM-17"
    );

    /* ========================================================
     * TASK 15: Valid scanf calls
     * ======================================================== */
    test_valid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%d\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf with %d and &int_variable"
    );

    test_valid_io(
        "int main() {\n"
        "    float f;\n"
        "    scanf(\"%f\", &f);\n"
        "    return 0;\n"
        "}\n",
        "scanf with %f and &float_variable"
    );

    test_valid_io(
        "int main() {\n"
        "    char c;\n"
        "    scanf(\"%c\", &c);\n"
        "    return 0;\n"
        "}\n",
        "scanf with %c and &char_variable"
    );

    test_valid_io(
        "int main() {\n"
        "    int x;\n"
        "    float f;\n"
        "    char c;\n"
        "    scanf(\"%d %f %c\", &x, &f, &c);\n"
        "    return 0;\n"
        "}\n",
        "scanf with multiple specifiers (%d %f %c)"
    );

    /* ========================================================
     * TASK 16: Invalid scanf calls (SEM-18)
     * ======================================================== */
    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(x);\n"
        "    return 0;\n"
        "}\n",
        "scanf first argument is variable instead of string literal",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%d\", x);\n"
        "    return 0;\n"
        "}\n",
        "scanf destination missing address-of operator &",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    float f;\n"
        "    scanf(\"%d\", &f);\n"
        "    return 0;\n"
        "}\n",
        "scanf %d with float destination mismatch",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%f\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf %f with int destination mismatch",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%c\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf %c with int destination mismatch",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%d %f\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf format requires 2 destinations, got 1",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    int y;\n"
        "    scanf(\"%d\", &x, &y);\n"
        "    return 0;\n"
        "}\n",
        "scanf format requires 1 destination, got 2",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%s\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf with unsupported %s specifier",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int x;\n"
        "    scanf(\"%x\", &x);\n"
        "    return 0;\n"
        "}\n",
        "scanf with unsupported %x specifier",
        "SEM-18"
    );

    /* ========================================================
     * TASK 17: Ampersand operator restrictions (SEM-18)
     * ======================================================== */
    test_invalid_io(
        "int main() {\n"
        "    int x = 10;\n"
        "    int y;\n"
        "    y = &x;\n"
        "    return 0;\n"
        "}\n",
        "address-of operator & in assignment expression",
        "SEM-18"
    );

    test_invalid_io(
        "void foo(int a) {}\n"
        "int main() {\n"
        "    int x = 10;\n"
        "    foo(&x);\n"
        "    return 0;\n"
        "}\n",
        "address-of operator & in ordinary function call",
        "SEM-18"
    );

    test_invalid_io(
        "int main() {\n"
        "    int y = 5;\n"
        "    int x;\n"
        "    x = &(y + 1);\n"
        "    return 0;\n"
        "}\n",
        "address-of operator & on non-lvalue expression",
        "SEM-18"
    );

    /* ========================================================
     * TASK 18: Integration Programs
     * ======================================================== */
    test_valid_io(
        "int calculate_sum(int a, int b, int c) {\n"
        "    return a + b + c;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int scores[3];\n"
        "    int i;\n"
        "    printf(\"Please enter 3 scores:\\n\");\n"
        "    for (i = 0; i < 3; i++) {\n"
        "        int s;\n"
        "        scanf(\"%d\", &s);\n"
        "        scores[i] = s;\n"
        "    }\n"
        "    int total = calculate_sum(scores[0], scores[1], scores[2]);\n"
        "    printf(\"Total = %d\\n\", total);\n"
        "    return 0;\n"
        "}\n",
        "Full valid integration program with printf, scanf, functions, arrays, and loops"
    );

    test_multi_error_recovery(
        "void helper(int a) {}\n"
        "int main() {\n"
        "    int x = 10;\n"
        "    float f = 2.5;\n"
        "    printf(\"%d\", f);\n"          /* Error 1: printf %d with float */
        "    printf(\"%x\", x);\n"          /* Error 2: printf unsupported %x */
        "    scanf(\"%d\", &f);\n"           /* Error 3: scanf %d with float */
        "    scanf(\"%s\", &x);\n"           /* Error 4: scanf unsupported %s */
        "    helper(&x);\n"                  /* Error 5: &x outside scanf */
        "    return 0;\n"
        "}\n",
        "Multi-error program verifying compiler recovery and multiple diagnostics",
        5
    );

    printf("====================================================\n");
    printf("STAGE 10 TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
