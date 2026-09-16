#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbol_table.h"
#include "semantic.h"

static int total_tests = 0;
static int passed_tests = 0;

static void test_scope_build_from_string(const char *src, const char *desc, int expect_errors) {
    total_tests++;
    printf("====================================================\n");
    printf("Scope Build Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<test_prog>");
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing failed with %d error(s)\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int errors = semantic_build_symbols(ctx, ast);

    printf("Scope Tree & Populated Symbol Table:\n");
    scope_print(ctx->global_scope, 1);

    if (expect_errors) {
        if (errors > 0) {
            printf("\nRESULT: SUCCESS (Caught %d semantic error(s) as expected)\n", errors);
            passed_tests++;
        } else {
            printf("\nRESULT: FAILED (Expected semantic errors, but build succeeded!)\n");
        }
    } else {
        if (errors == 0) {
            printf("\nRESULT: SUCCESS (0 semantic errors, scope hierarchy successfully populated)\n");
            passed_tests++;
        } else {
            printf("\nRESULT: FAILED (%d semantic error(s) encountered)\n", errors);
        }
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_scope_build_from_file(const char *filepath, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Scope Build File Test %02d [%s]: %s\n", total_tests, desc, filepath);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_file(filepath);
    if (!lexer) {
        printf("FAILED: Could not open source file '%s'\n", filepath);
        return;
    }

    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing failed with %d error(s)\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int errors = semantic_build_symbols(ctx, ast);

    printf("Scope Tree & Populated Symbol Table for %s:\n", filepath);
    scope_print(ctx->global_scope, 1);

    if (errors == 0) {
        printf("\nRESULT: SUCCESS (0 semantic errors, scope hierarchy successfully populated)\n");
        passed_tests++;
    } else {
        printf("\nRESULT: FAILED (%d semantic error(s) encountered)\n", errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

int main(void) {
    printf("####################################################\n");
    printf("#     STAGE 7: SCOPE ANALYSIS INTEGRATION TEST    #\n");
    printf("####################################################\n\n");

    /* Test 1: Task 11 Specification Program */
    const char *task11_program =
        "int global = 10;\n"
        "\n"
        "int add(int a, int b) {\n"
        "    int result = a + b;\n"
        "\n"
        "    {\n"
        "        int local = 20;\n"
        "        result = result + local;\n"
        "    }\n"
        "\n"
        "    return result;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int x = add(global, 5);\n"
        "\n"
        "    {\n"
        "        int global = 100;\n"
        "        x = x + global;\n"
        "    }\n"
        "\n"
        "    return x;\n"
        "}\n";

    test_scope_build_from_string(task11_program, "Task 11 Specification Program (Shadowing & Scopes)", 0);

    /* Test 2: Conformance file examples/test.c */
    test_scope_build_from_file("examples/test.c", "Conformance Test File");

    /* Test 3: Duplicate Global Declaration (SEM-02 Error Test) */
    const char *dup_global_prog =
        "int count = 0;\n"
        "int count = 5;\n"
        "int main() {\n"
        "    return 0;\n"
        "}\n";
    test_scope_build_from_string(dup_global_prog, "Duplicate Global Declaration Error", 1);

    /* Test 4: Duplicate Local Declaration in Same Scope (SEM-02 Error Test) */
    const char *dup_local_prog =
        "int main() {\n"
        "    int x = 1;\n"
        "    int x = 2;\n"
        "    return x;\n"
        "}\n";
    test_scope_build_from_string(dup_local_prog, "Duplicate Local Declaration in Same Scope Error", 1);

    /* Test 5: Duplicate Parameter Name (SEM-02 Error Test) */
    const char *dup_param_prog =
        "int calc(int a, int a) {\n"
        "    return a;\n"
        "}\n"
        "int main() {\n"
        "    return 0;\n"
        "}\n";
    test_scope_build_from_string(dup_param_prog, "Duplicate Function Parameter Error", 1);

    printf("====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
