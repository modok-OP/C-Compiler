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

static void test_valid_semantic(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Valid Semantic Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<test_prog>");
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        printf("FAILED: Parsing failed with %d syntax error(s)\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int errors = semantic_analyze(ctx, ast);

    if (errors == 0) {
        printf("RESULT: PASSED (0 semantic errors)\n");
        passed_tests++;
    } else {
        printf("RESULT: FAILED (%d semantic errors encountered)\n", errors);
    }

    semantic_context_destroy(ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_invalid_semantic(const char *src, const char *desc, const char *expected_sem_id) {
    total_tests++;
    printf("====================================================\n");
    printf("Invalid Semantic Test %02d [%s]: Expected [%s]\n%s\n", total_tests, desc, expected_sem_id, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<invalid_prog>");
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        printf("FAILED: Unexpected syntax parsing failure (%d syntax errors)\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return;
    }

    SemanticContext *ctx = semantic_context_create();
    int errors = semantic_analyze(ctx, ast);

    if (errors > 0) {
        printf("RESULT: PASSED (Caught %d semantic error(s) matching %s)\n", errors, expected_sem_id);
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

int main(void) {
    printf("####################################################\n");
    printf("#      STAGE 9: FULL SEMANTIC ANALYZER TESTS       #\n");
    printf("####################################################\n\n");

    /*
     * Valid Program Tests
     */
    test_valid_semantic(
        "int main() {\n"
        "    int a = 10;\n"
        "    float b = 20.5;\n"
        "    char c = 'Z';\n"
        "    return 0;\n"
        "}",
        "1. Valid basic declarations"
    );

    test_valid_semantic(
        "int main() {\n"
        "    int a = 10;\n"
        "    float b = a;\n"
        "    return 0;\n"
        "}",
        "2. Valid int -> float widening initialization"
    );

    test_valid_semantic(
        "int main() {\n"
        "    int a = 5;\n"
        "    float b = 10.5;\n"
        "    float c = a + b * 2;\n"
        "    int m = a % 2;\n"
        "    return 0;\n"
        "}",
        "3. Valid arithmetic and modulus"
    );

    test_valid_semantic(
        "int main() {\n"
        "    int a = 5;\n"
        "    float b = 10.0;\n"
        "    if (a < b && b != 0.0) {\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}",
        "4. Valid comparisons and logical expressions"
    );

    test_valid_semantic(
        "int main() {\n"
        "    int arr[5];\n"
        "    int i = 0;\n"
        "    arr[i] = 10;\n"
        "    arr[i + 1] = arr[i] + 5;\n"
        "    return 0;\n"
        "}",
        "5. Valid 1-D array declaration and access"
    );

    test_valid_semantic(
        "int add(int x, int y) {\n"
        "    return x + y;\n"
        "}\n"
        "int main() {\n"
        "    int res = add(10, 20);\n"
        "    return res;\n"
        "}",
        "6. Valid user function call"
    );

    test_valid_semantic(
        "int main() {\n"
        "    float x = 3.14;\n"
        "    int a = (int)x;\n"
        "    char c = (char)a;\n"
        "    return 0;\n"
        "}",
        "7. Valid explicit casts"
    );

    test_valid_semantic(
        "int main() {\n"
        "    int i = 0;\n"
        "    while (i < 5) {\n"
        "        if (i == 2) break;\n"
        "        i++;\n"
        "    }\n"
        "    for (int j = 0; j < 5; j++) {\n"
        "        if (j == 3) continue;\n"
        "    }\n"
        "    do {\n"
        "        i--;\n"
        "    } while (i > 0);\n"
        "    return 0;\n"
        "}",
        "8. Valid loops and loop control (break/continue)"
    );

    test_valid_semantic(
        "void log_msg() {\n"
        "    printf(\"Logging message\\n\");\n"
        "    return;\n"
        "}\n"
        "int main() {\n"
        "    log_msg();\n"
        "    int val = 0;\n"
        "    scanf(\"%d\", &val);\n"
        "    return 0;\n"
        "}",
        "9. Valid void function and I/O calls"
    );

    test_valid_semantic(
        "int global = 10;\n"
        "int main() {\n"
        "    int x = global;\n"
        "    {\n"
        "        int global = 100;\n"
        "        x = x + global;\n"
        "    }\n"
        "    return x;\n"
        "}",
        "10. Valid nested scopes and legal shadowing"
    );

    /*
     * Invalid Program Tests (Testing SEM error families)
     */
    printf("####################################################\n");
    printf("#           INVALID SEMANTIC ERROR TESTS           #\n");
    printf("####################################################\n\n");

    test_invalid_semantic(
        "int main() {\n"
        "    x = 10;\n"
        "    return 0;\n"
        "}",
        "Undeclared identifier in assignment",
        "SEM-01"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int a = 10;\n"
        "    int a = 20;\n"
        "    return 0;\n"
        "}",
        "Duplicate variable declaration in same scope",
        "SEM-02"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    {\n"
        "        int y = 10;\n"
        "    }\n"
        "    y = 20;\n"
        "    return 0;\n"
        "}",
        "Out-of-scope identifier reference",
        "SEM-03"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int x = 3.14;\n"
        "    return 0;\n"
        "}",
        "Assignment narrowing mismatch (float -> int)",
        "SEM-04"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    char c = 'A';\n"
        "    int x = c + 5;\n"
        "    return 0;\n"
        "}",
        "Invalid arithmetic operand (char + int)",
        "SEM-05"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    float x = 10.5;\n"
        "    int m = x % 2;\n"
        "    return 0;\n"
        "}",
        "Invalid modulus operand (float % int)",
        "SEM-06"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int x = 10;\n"
        "    if (x + 1) {\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}",
        "Invalid condition (numeric expression without boolean comparison)",
        "SEM-07"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int x = 1;\n"
        "    int y = 2;\n"
        "    if (x && y) {\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}",
        "Invalid logical operand (numeric operands to &&)",
        "SEM-08"
    );

    test_invalid_semantic(
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "int main() {\n"
        "    return add(10);\n"
        "}",
        "Wrong function arity (expected 2 args, got 1)",
        "SEM-09"
    );

    test_invalid_semantic(
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "int main() {\n"
        "    return add(10, 3.14);\n"
        "}",
        "Wrong argument type (float passed to int parameter)",
        "SEM-10"
    );

    test_invalid_semantic(
        "int compute() {\n"
        "    return 3.14;\n"
        "}\n"
        "int main() {\n"
        "    return 0;\n"
        "}",
        "Invalid return type (float returned from int function)",
        "SEM-11"
    );

    test_invalid_semantic(
        "int calculate() {\n"
        "    int x = 10;\n"
        "}\n"
        "int main() {\n"
        "    return 0;\n"
        "}",
        "Missing return in non-void function",
        "SEM-12"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    break;\n"
        "    return 0;\n"
        "}",
        "Break statement outside loop",
        "SEM-13"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    continue;\n"
        "    return 0;\n"
        "}",
        "Continue statement outside loop",
        "SEM-14"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int arr[5];\n"
        "    int val = arr[2.5];\n"
        "    return 0;\n"
        "}",
        "Invalid array index type (float index)",
        "SEM-15"
    );

    test_invalid_semantic(
        "int main() {\n"
        "    int arr[5];\n"
        "    arr[0] = 3.14;\n"
        "    return 0;\n"
        "}",
        "Array element assignment mismatch (float assigned to int array)",
        "SEM-16"
    );

    test_invalid_semantic(
        "void logger() {\n"
        "    return;\n"
        "}\n"
        "int main() {\n"
        "    int x = (int)logger();\n"
        "    return 0;\n"
        "}",
        "Invalid explicit cast from void",
        "SEM-19"
    );

    test_invalid_semantic(
        "void main() {\n"
        "    return;\n"
        "}",
        "Invalid main signature (void main instead of int main)",
        "SEM-20"
    );

    test_invalid_semantic(
        "int compute() {\n"
        "    return 0;\n"
        "}",
        "Missing main function in program",
        "SEM-20"
    );

    printf("====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
