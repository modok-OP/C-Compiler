#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

static int total_tests = 0;
static int passed_tests = 0;

static void test_expr(const char *expr_str) {
    total_tests++;
    printf("====================================================\n");
    printf("Test %02d: %s\n", total_tests, expr_str);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(expr_str, "<test>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_expression(parser);

    if (ast && parser->error_count == 0) {
        printf("AST Structure:\n");
        ast_print(ast, 1);
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

static void test_error_expr(const char *expr_str, const char *scenario) {
    total_tests++;
    printf("====================================================\n");
    printf("Error Test %02d [%s]: %s\n", total_tests, scenario, expr_str);
    printf("----------------------------------------------------\n");

    Lexer *lexer = lexer_create_from_string(expr_str, "<error_test>");
    Parser *parser = parser_create(lexer);

    ASTNode *ast = parser_parse_expression(parser);

    if (parser->error_count > 0) {
        printf("SUCCESS: Caught %d syntax error(s) gracefully without crashing.\n", parser->error_count);
        passed_tests++;
    } else {
        printf("FAILED: Expected syntax error, but expression parsed!\n");
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
    printf("#     STAGE 4: EXPRESSION PARSER VERIFICATION     #\n");
    printf("####################################################\n\n");

    /* 1. Literals and Identifiers */
    test_expr("42");
    test_expr("3.1415");
    test_expr("'A'");
    test_expr("\"Hello, World!\"");
    test_expr("identifier_name");

    /* 2. Parentheses and Precedence */
    test_expr("a + b * c");
    test_expr("(a + b) * c");

    /* 3. Prefix and Postfix Increments/Decrements */
    test_expr("x++");
    test_expr("++x");
    test_expr("y--");
    test_expr("--y");

    /* 4. Unary Operators */
    test_expr("+a");
    test_expr("-b");
    test_expr("!flag");
    test_expr("&dest");

    /* 5. Array Subscripting */
    test_expr("arr[i]");
    test_expr("matrix[row + 1]");

    /* 6. Function Calls */
    test_expr("foo()");
    test_expr("foo(a, b + 1)");
    test_expr("printf(\"Result = %d\\n\", total)");

    /* 7. Type Casts */
    test_expr("(int)x");
    test_expr("(float)a");
    test_expr("(char)n");

    /* 8. Arithmetic Associativity */
    test_expr("a * b / c % d");
    test_expr("a + b - c + d");

    /* 9. Relational and Equality Operators */
    test_expr("a < b");
    test_expr("x <= y");
    test_expr("a > b");
    test_expr("x >= y");
    test_expr("a == b");
    test_expr("a != b");

    /* 10. Logical Operators and Mixed Precedence */
    test_expr("a < b && b != c");
    test_expr("x || y && z");

    /* 11. Assignment and Compound Assignment (Right-Associative) */
    test_expr("a = 10");
    test_expr("a += 5");
    test_expr("b -= 2");
    test_expr("c *= 3");
    test_expr("d /= 4");
    test_expr("e %= 2");
    test_expr("a = b = 10");
    test_expr("x += y = 20");

    /* 12. Complex Mixed Expressions */
    test_expr("arr[i++] = (int)foo(a + 2, b * 3) + 10");

    printf("####################################################\n");
    printf("#            SYNTAX ERROR ROBUSTNESS TESTS         #\n");
    printf("####################################################\n\n");

    test_error_expr("a +", "Trailing binary operator");
    test_error_expr("(a + b", "Unclosed parenthesis");
    test_error_expr("foo(", "Unclosed function call");
    test_error_expr("a =", "Missing RHS in assignment");
    test_error_expr("++", "Missing operand for prefix operator");
    test_error_expr("arr[i", "Unclosed array bracket");
    test_error_expr("(int x", "Malformed cast expression");

    printf("####################################################\n");
    printf("Summary: %d / %d tests passed successfully.\n", passed_tests, total_tests);
    printf("####################################################\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
