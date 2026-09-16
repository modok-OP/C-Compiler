#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

static int total_tests = 0;
static int passed_tests = 0;

static void test_statement(const char *src, const char *desc) {
    total_tests++;
    printf("====================================================\n");
    printf("Test %02d [%s]:\n%s\n", total_tests, desc, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<test>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_statement(parser);

    if (parser->error_count == 0) {
        printf("AST Structure:\n");
        if (ast) {
            ast_print(ast, 1);
        } else {
            printf("  (Empty Statement)\n");
        }
        passed_tests++;
    } else {
        printf("FAILED: Parsing failed with %d error(s).\n", parser->error_count);
    }

    if (ast) {
        ast_free(ast);
    }
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

static void test_error_statement(const char *src, const char *scenario) {
    total_tests++;
    printf("====================================================\n");
    printf("Error Test %02d [%s]:\n%s\n", total_tests, scenario, src);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(src, "<error_test>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_statement(parser);

    if (parser->error_count > 0) {
        printf("SUCCESS: Caught %d syntax error(s) gracefully without crashing.\n", parser->error_count);
        passed_tests++;
    } else {
        printf("FAILED: Expected syntax error, but statement parsed successfully!\n");
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
    printf("#     STAGE 5: STATEMENTS & DECLARATIONS TESTS     #\n");
    printf("####################################################\n\n");

    /* 1. Variable Declarations */
    test_statement("int x;", "Uninitialized scalar declaration");
    test_statement("float price;", "Float scalar declaration");
    test_statement("char grade;", "Char scalar declaration");

    /* 2. Initialized Declarations */
    test_statement("int count = 10;", "Initialized int declaration");
    test_statement("float ratio = 3.14;", "Initialized float declaration");
    test_statement("char initial = 'A';", "Initialized char declaration");

    /* 3. Multiple Declarators */
    test_statement("int a, b, c;", "Multiple uninitialized declarators");
    test_statement("int x = 1, y = 2, z;", "Multiple mixed initialized declarators");

    /* 4. Array Declarations */
    test_statement("int marks[5];", "Int 1-D array declaration");
    test_statement("float values[10];", "Float 1-D array declaration");
    test_statement("char letters[26];", "Char 1-D array declaration");

    /* 5. Expression Statements */
    test_statement("x = x + 5;", "Assignment expression statement");
    test_statement("printf(\"Hello, World!\\n\");", "Function call expression statement");
    test_statement("x++;", "Postfix increment expression statement");

    /* 6. Empty Statement */
    test_statement(";", "Empty statement (standalone semicolon)");

    /* 7. Blocks */
    test_statement("{\n  int x = 1;\n  x++;\n}", "Compound statement block");

    /* 8. Return with expression */
    test_statement("return 0;", "Return integer literal");
    test_statement("return a + b;", "Return binary expression");

    /* 9. Return without expression */
    test_statement("return;", "Void return");

    /* 10. Break Statement */
    test_statement("break;", "Break statement");

    /* 11. Continue Statement */
    test_statement("continue;", "Continue statement");

    /* 12. If Statement */
    test_statement("if (x > 0) x++;", "Single if statement without else");

    /* 13. If / Else Statement */
    test_statement("if (x > 0) x++; else x--;", "If statement with else branch");

    /* 14. Nested If / Else (Dangling-Else Resolution) */
    test_statement("if (a) if (b) x = 1; else x = 2;", "Nested if-else associating else with nearest if");

    /* 15. While Statement */
    test_statement("while (x < 100) x++;", "While loop with simple body");
    test_statement("while (x < 100) {\n  total += x;\n  x++;\n}", "While loop with block body");

    /* 16. Do-While Statement */
    test_statement("do x++; while (x < 100);", "Do-while loop with single statement");
    test_statement("do {\n  x++;\n} while (x < 100);", "Do-while loop with block body");

    /* 17. For Loop with Declaration */
    test_statement("for (int i = 0; i < 10; i++) x += i;", "For loop with int declaration init");

    /* 18. For Loop with Expression Initialization */
    test_statement("for (i = 0; i < 10; i++) x += i;", "For loop with assignment init");

    /* 19. For Loop with Omitted Clauses */
    test_statement("for (;;) break;", "For loop with omitted init, cond, and update");
    test_statement("for (; i < 10;) i++;", "For loop with condition only");

    /* 20. Nested Loops */
    test_statement("while (i < 10) {\n  for (j = 0; j < 5; j++) {\n    if (i == j) continue;\n  }\n  i++;\n}",
                   "Nested while and for loops with continue");

    printf("####################################################\n");
    printf("#            SYNTAX ERROR ROBUSTNESS TESTS         #\n");
    printf("####################################################\n\n");

    /* Malformed Error Tests from Task 16 */
    test_error_statement("int x", "Missing semicolon in variable declaration");
    test_error_statement("int x =", "Missing expression in initialized declaration");
    test_error_statement("int a[;", "Missing array size in array declaration");
    test_error_statement("if (x", "Unclosed parenthesis in if statement");
    test_error_statement("while (x", "Unclosed parenthesis in while statement");
    test_error_statement("for (int i = 0; i < 10; )", "Unclosed parenthesis in for statement");
    test_error_statement("return", "Missing semicolon in return statement");
    test_error_statement("break", "Missing semicolon in break statement");
    test_error_statement("continue", "Missing semicolon in continue statement");

    printf("####################################################\n");
    printf("Summary: %d / %d tests passed successfully.\n", passed_tests, total_tests);
    printf("####################################################\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
