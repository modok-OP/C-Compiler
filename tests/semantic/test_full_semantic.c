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

static void test_pipeline(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Pipeline Integration Test %02d [%s]:\n", total_tests, desc);
    printf("----------------------------------------------------\n");

    /* 1. Lexer */
    Lexer *lexer = lexer_create_from_string(src, "<pipeline_test>");
    if (!lexer) {
        printf("FAILED: Lexer initialization failed\n\n");
        return;
    }

    /* 2. Parser */
    Parser *parser = parser_create(lexer);
    if (!parser) {
        printf("FAILED: Parser initialization failed\n\n");
        lexer_destroy(lexer);
        return;
    }

    /* 3. AST Generation */
    ASTNode *ast = parser_parse_program(parser);
    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing encountered %d error(s)\n\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }
    printf("[1/3] Lexical and Syntax Analysis: SUCCESS (0 errors)\n");

    /* 4. Symbol Table & Full Semantic Analysis */
    SemanticContext *ctx = semantic_context_create();
    int sem_errors = semantic_analyze(ctx, ast);

    if (sem_errors == 0) {
        printf("[2/3] Symbol Table & Scope Resolution: SUCCESS\n");
        printf("[3/3] Semantic Validation & Type Checking: SUCCESS (0 errors)\n");
        printf("\nRESULT: PASSED (End-to-End Pipeline Complete)\n");
        passed_tests++;
    } else {
        printf("[x] Semantic Validation FAILED with %d error(s)\n", sem_errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_pipeline_file(const char *filepath, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Pipeline File Integration Test %02d [%s]: %s\n", total_tests, desc, filepath);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_file(filepath);
    if (!lexer) {
        printf("FAILED: Could not open source file '%s'\n\n", filepath);
        return;
    }

    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);
    if (!ast || parser->error_count > 0) {
        printf("FAILED: Syntax parsing encountered %d error(s)\n\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }
    printf("[1/3] Lexical and Syntax Analysis: SUCCESS (0 errors)\n");

    SemanticContext *ctx = semantic_context_create();
    int sem_errors = semantic_analyze(ctx, ast);

    if (sem_errors == 0) {
        printf("[2/3] Symbol Table & Scope Resolution: SUCCESS\n");
        printf("[3/3] Semantic Validation & Type Checking: SUCCESS (0 errors)\n");
        printf("\nRESULT: PASSED (End-to-End Pipeline Complete for %s)\n", filepath);
        passed_tests++;
    } else {
        printf("[x] Semantic Validation FAILED with %d error(s)\n", sem_errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

int main(void) {
    printf("####################################################\n");
    printf("#     STAGE 9: END-TO-END PIPELINE INTEGRATION     #\n");
    printf("####################################################\n\n");

    /* Comprehensive program combining all features */
    const char *full_program =
        "int global_multiplier = 2;\n"
        "\n"
        "float compute(float base, int exp) {\n"
        "    float result = 1.0;\n"
        "    int i;\n"
        "    for (i = 0; i < exp; i++) {\n"
        "        result *= base;\n"
        "    }\n"
        "    return result;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int numbers[4];\n"
        "    int idx;\n"
        "    int sum = 0;\n"
        "\n"
        "    for (idx = 0; idx < 4; idx++) {\n"
        "        numbers[idx] = (idx + 1) * global_multiplier;\n"
        "    }\n"
        "\n"
        "    idx = 0;\n"
        "    while (idx < 4) {\n"
        "        sum += numbers[idx];\n"
        "        idx++;\n"
        "    }\n"
        "\n"
        "    float power = compute((float)sum, 2);\n"
        "\n"
        "    {\n"
        "        int local_flag = 1;\n"
        "        if (power > 50.0 && local_flag == 1) {\n"
        "            printf(\"Result is large\\n\");\n"
        "        }\n"
        "    }\n"
        "\n"
        "    return 0;\n"
        "}\n";

    test_pipeline(full_program, "Comprehensive C Program Feature Integration");
    test_pipeline_file("examples/test.c", "Specification Conformance File");

    printf("====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
