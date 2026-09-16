#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

static int total_tests = 0;
static int passed_tests = 0;

static void test_valid_program_string(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Valid Program Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<test_prog>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_program(parser);

    if (parser->error_count == 0 && ast != NULL) {
        printf("AST Structure:\n");
        ast_print(ast, 1);
        printf("\nRESULT: SUCCESS (0 errors)\n");
        passed_tests++;
    } else {
        printf("RESULT: FAILED (Errors: %d, AST: %p)\n", parser->error_count, (void *)ast);
    }

    if (ast) {
        ast_free(ast);
    }
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_valid_program_file(const char *filepath, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Valid Program File Test %02d [%s]: %s\n", total_tests, desc, filepath);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_file(filepath);
    if (!lexer) {
        printf("RESULT: FAILED (Could not open file '%s')\n\n", filepath);
        return;
    }

    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (parser->error_count == 0 && ast != NULL) {
        printf("AST Structure:\n");
        ast_print(ast, 1);
        printf("\nRESULT: SUCCESS (0 errors)\n");
        passed_tests++;
    } else {
        printf("RESULT: FAILED (Errors: %d, AST: %p)\n", parser->error_count, (void *)ast);
    }

    if (ast) {
        ast_free(ast);
    }
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_error_program_string(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Error Program Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<error_prog>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_program(parser);

    if (parser->error_count > 0) {
        printf("SUCCESS: Caught %d syntax error(s) gracefully without crashing.\n", parser->error_count);
        passed_tests++;
    } else {
        printf("FAILED: Expected syntax error, but program parsed successfully!\n");
    }

    if (ast) {
        ast_free(ast);
    }
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

int main(void) {
    printf("####################################################\n");
    printf("#   STAGE 6: PROGRAM & FUNCTION DEFINITION TESTS   #\n");
    printf("####################################################\n\n");

    /*
     * Valid Program Tests (Task 10)
     */
    test_valid_program_string(
        "int main() {\n"
        "    int x = 10;\n"
        "    x = x + 5;\n"
        "    return x;\n"
        "}",
        "Program 1: Minimal main"
    );

    test_valid_program_string(
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int result = add(10, 20);\n"
        "    printf(\"result = %d\\n\", result);\n"
        "    return 0;\n"
        "}",
        "Program 2: Function with arguments and function call"
    );

    test_valid_program_string(
        "int global = 10;\n"
        "\n"
        "float average(float a, float b) {\n"
        "    return (a + b) / 2;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    float x = average(10.0, 20.0);\n"
        "    printf(\"%f\\n\", x);\n"
        "    return 0;\n"
        "}",
        "Program 3: Global declarations and functions"
    );

    test_valid_program_string(
        "void display() {\n"
        "    printf(\"Hello\\n\");\n"
        "    return;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    display();\n"
        "    return 0;\n"
        "}",
        "Program 4: Void function"
    );

    test_valid_program_file(
        "examples/test.c",
        "Program 5: Specification conformance file (examples/test.c)"
    );

    /*
     * Malformed Error Program Tests (Task 12)
     */
    printf("####################################################\n");
    printf("#          MALFORMED ERROR PROGRAM TESTS           #\n");
    printf("####################################################\n\n");

    test_error_program_string(
        "int main()",
        "Error 1: Missing function body"
    );

    test_error_program_string(
        "int main() {\n"
        "    return 0;",
        "Error 2: Missing closing brace in function body"
    );

    test_error_program_string(
        "int add(int, int b) {\n"
        "    return 0;\n"
        "}",
        "Error 3: Missing parameter name"
    );

    test_error_program_string(
        "int add(int a int b) {\n"
        "    return 0;\n"
        "}",
        "Error 4: Missing comma between parameters"
    );

    test_error_program_string(
        "main() {\n"
        "    return 0;\n"
        "}",
        "Error 5: Missing return type for main"
    );

    test_error_program_string(
        "int @x;",
        "Error 6: Invalid top-level token / lexical error"
    );

    test_error_program_string(
        "int foo(int a)",
        "Error 7: Incomplete function definition without body"
    );

    test_error_program_string(
        "int main(int argc) {\n"
        "    return 0;\n"
        "}",
        "Error 8: Parameter in main function"
    );

    test_error_program_string(
        "return 0;",
        "Error 9: Statement outside function at top level"
    );

    printf("====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
