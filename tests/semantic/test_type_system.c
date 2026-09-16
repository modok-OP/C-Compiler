#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_system.h"

static int total_tests = 0;
static int passed_tests = 0;

static void assert_test(int condition, const char *test_name) {
    total_tests++;
    printf("Test %02d [%s]: ", total_tests, test_name);
    if (condition) {
        printf("PASSED\n");
        passed_tests++;
    } else {
        printf("FAILED\n");
    }
}

int main(void) {
    printf("####################################################\n");
    printf("#     STAGE 8: TYPE SYSTEM & CONVERSIONS TESTS     #\n");
    printf("####################################################\n\n");

    /* Test 1: Same-type compatibility */
    int t1 = type_can_implicitly_convert(SEM_TYPE_INT, SEM_TYPE_INT) &&
             type_can_implicitly_convert(SEM_TYPE_FLOAT, SEM_TYPE_FLOAT) &&
             type_can_implicitly_convert(SEM_TYPE_CHAR, SEM_TYPE_CHAR);
    assert_test(t1, "1. Same-type compatibility");

    /* Test 2: int -> float widening */
    int t2 = type_can_implicitly_convert(SEM_TYPE_INT, SEM_TYPE_FLOAT);
    assert_test(t2 == 1, "2. int -> float widening allowed implicitly");

    /* Test 3: float -> int rejection */
    int t3 = type_can_implicitly_convert(SEM_TYPE_FLOAT, SEM_TYPE_INT);
    assert_test(t3 == 0, "3. float -> int narrowing rejected implicitly");

    /* Test 4: int -> char rejection */
    int t4 = type_can_implicitly_convert(SEM_TYPE_INT, SEM_TYPE_CHAR);
    assert_test(t4 == 0, "4. int -> char cross-type conversion rejected");

    /* Test 5: char -> int rejection */
    int t5 = type_can_implicitly_convert(SEM_TYPE_CHAR, SEM_TYPE_INT);
    assert_test(t5 == 0, "5. char -> int promotion rejected (char kept distinct)");

    /* Test 6: char -> float rejection */
    int t6 = type_can_implicitly_convert(SEM_TYPE_CHAR, SEM_TYPE_FLOAT);
    assert_test(t6 == 0, "6. char -> float conversion rejected");

    /* Test 7: float -> char rejection */
    int t7 = type_can_implicitly_convert(SEM_TYPE_FLOAT, SEM_TYPE_CHAR);
    assert_test(t7 == 0, "7. float -> char conversion rejected");

    /* Test 8: void conversions rejection */
    int t8 = (!type_can_implicitly_convert(SEM_TYPE_VOID, SEM_TYPE_INT)) &&
             (!type_can_implicitly_convert(SEM_TYPE_INT, SEM_TYPE_VOID)) &&
             (!type_can_implicitly_convert(SEM_TYPE_VOID, SEM_TYPE_VOID));
    assert_test(t8, "8. void conversions rejected for value types");

    /* Test 9: Explicit casts between int, float, char */
    int t9 = type_can_explicit_cast(SEM_TYPE_INT, SEM_TYPE_FLOAT) &&
             type_can_explicit_cast(SEM_TYPE_FLOAT, SEM_TYPE_INT) &&
             type_can_explicit_cast(SEM_TYPE_CHAR, SEM_TYPE_INT) &&
             type_can_explicit_cast(SEM_TYPE_INT, SEM_TYPE_CHAR) &&
             type_can_explicit_cast(SEM_TYPE_FLOAT, SEM_TYPE_CHAR) &&
             type_can_explicit_cast(SEM_TYPE_CHAR, SEM_TYPE_FLOAT) &&
             (!type_can_explicit_cast(SEM_TYPE_VOID, SEM_TYPE_INT)) &&
             (!type_can_explicit_cast(SEM_TYPE_INT, SEM_TYPE_VOID));
    assert_test(t9, "9. Explicit casts supported for int/float/char; void rejected");

    /* Test 10: Arithmetic result types */
    int t10 = (type_arithmetic_result(TOKEN_PLUS, SEM_TYPE_INT, SEM_TYPE_INT) == SEM_TYPE_INT) &&
              (type_arithmetic_result(TOKEN_PLUS, SEM_TYPE_INT, SEM_TYPE_FLOAT) == SEM_TYPE_FLOAT) &&
              (type_arithmetic_result(TOKEN_MULTIPLY, SEM_TYPE_FLOAT, SEM_TYPE_INT) == SEM_TYPE_FLOAT) &&
              (type_arithmetic_result(TOKEN_DIVIDE, SEM_TYPE_FLOAT, SEM_TYPE_FLOAT) == SEM_TYPE_FLOAT);
    assert_test(t10, "10. Arithmetic result types (+, -, *, /)");

    /* Test 11: Invalid char arithmetic */
    int t11 = (type_arithmetic_result(TOKEN_PLUS, SEM_TYPE_CHAR, SEM_TYPE_INT) == SEM_TYPE_ERROR) &&
              (type_arithmetic_result(TOKEN_MINUS, SEM_TYPE_INT, SEM_TYPE_CHAR) == SEM_TYPE_ERROR) &&
              (type_arithmetic_result(TOKEN_MULTIPLY, SEM_TYPE_CHAR, SEM_TYPE_CHAR) == SEM_TYPE_ERROR);
    assert_test(t11, "11. Invalid char arithmetic rejected (char not auto-promoted)");

    /* Test 12: Valid modulus (int % int) */
    int t12 = type_is_valid_modulus(SEM_TYPE_INT, SEM_TYPE_INT) &&
              (type_arithmetic_result(TOKEN_MODULO, SEM_TYPE_INT, SEM_TYPE_INT) == SEM_TYPE_INT);
    assert_test(t12, "12. Valid modulus int % int -> int");

    /* Test 13: Invalid modulus (involving float or char) */
    int t13 = (!type_is_valid_modulus(SEM_TYPE_FLOAT, SEM_TYPE_INT)) &&
              (!type_is_valid_modulus(SEM_TYPE_INT, SEM_TYPE_FLOAT)) &&
              (!type_is_valid_modulus(SEM_TYPE_CHAR, SEM_TYPE_INT)) &&
              (type_arithmetic_result(TOKEN_MODULO, SEM_TYPE_FLOAT, SEM_TYPE_INT) == SEM_TYPE_ERROR);
    assert_test(t13, "13. Invalid modulus with non-int operands rejected");

    /* Test 14: Valid comparisons */
    int t14 = type_is_valid_comparison(SEM_TYPE_INT, SEM_TYPE_INT) &&
              type_is_valid_comparison(SEM_TYPE_INT, SEM_TYPE_FLOAT) &&
              type_is_valid_comparison(SEM_TYPE_FLOAT, SEM_TYPE_INT) &&
              type_is_valid_comparison(SEM_TYPE_FLOAT, SEM_TYPE_FLOAT) &&
              type_is_valid_comparison(SEM_TYPE_CHAR, SEM_TYPE_CHAR) &&
              (type_comparison_result(TOKEN_LESS, SEM_TYPE_INT, SEM_TYPE_FLOAT) == SEM_TYPE_BOOL);
    assert_test(t14, "14. Valid comparisons produce internal boolean");

    /* Test 15: Invalid comparisons */
    int t15 = (!type_is_valid_comparison(SEM_TYPE_CHAR, SEM_TYPE_INT)) &&
              (!type_is_valid_comparison(SEM_TYPE_INT, SEM_TYPE_CHAR)) &&
              (!type_is_valid_comparison(SEM_TYPE_CHAR, SEM_TYPE_FLOAT)) &&
              (type_comparison_result(TOKEN_EQUAL, SEM_TYPE_CHAR, SEM_TYPE_INT) == SEM_TYPE_ERROR);
    assert_test(t15, "15. Invalid cross-type char comparisons rejected");

    /* Test 16: Valid logical operands */
    int t16 = type_is_valid_logical_operand(SEM_TYPE_BOOL) &&
              (type_logical_result(TOKEN_LOGICAL_AND, SEM_TYPE_BOOL, SEM_TYPE_BOOL) == SEM_TYPE_BOOL) &&
              (type_logical_result(TOKEN_LOGICAL_OR, SEM_TYPE_BOOL, SEM_TYPE_BOOL) == SEM_TYPE_BOOL) &&
              (type_unary_logical_result(TOKEN_LOGICAL_NOT, SEM_TYPE_BOOL) == SEM_TYPE_BOOL);
    assert_test(t16, "16. Valid logical operands (bool && bool, bool || bool, !bool)");

    /* Test 17: Invalid logical operands (no arbitrary C truthiness) */
    int t17 = (!type_is_valid_logical_operand(SEM_TYPE_INT)) &&
              (!type_is_valid_logical_operand(SEM_TYPE_FLOAT)) &&
              (!type_is_valid_logical_operand(SEM_TYPE_CHAR)) &&
              (type_logical_result(TOKEN_LOGICAL_AND, SEM_TYPE_INT, SEM_TYPE_INT) == SEM_TYPE_ERROR) &&
              (type_unary_logical_result(TOKEN_LOGICAL_NOT, SEM_TYPE_INT) == SEM_TYPE_ERROR);
    assert_test(t17, "17. Invalid logical operands rejected (no numeric truthiness)");

    /* Test 18: Valid increment/decrement types (int and float) */
    int t18 = type_is_valid_increment_type(SEM_TYPE_INT) &&
              type_is_valid_increment_type(SEM_TYPE_FLOAT);
    assert_test(t18, "18. Valid increment/decrement types (int and float)");

    /* Test 19: Invalid char increment/decrement */
    int t19 = (!type_is_valid_increment_type(SEM_TYPE_CHAR)) &&
              (!type_is_valid_increment_type(SEM_TYPE_VOID)) &&
              (!type_is_valid_increment_type(SEM_TYPE_BOOL));
    assert_test(t19, "19. Invalid char increment/decrement rejected");

    /* Test 20: Assignment and compound assignment compatibility */
    int t20 = type_is_valid_assignment(SEM_TYPE_FLOAT, SEM_TYPE_INT) &&
              (!type_is_valid_assignment(SEM_TYPE_INT, SEM_TYPE_FLOAT)) &&
              type_is_valid_compound_assignment(TOKEN_PLUS_ASSIGN, SEM_TYPE_FLOAT, SEM_TYPE_INT) &&
              (!type_is_valid_compound_assignment(TOKEN_PLUS_ASSIGN, SEM_TYPE_INT, SEM_TYPE_FLOAT)) &&
              type_is_valid_compound_assignment(TOKEN_MODULO_ASSIGN, SEM_TYPE_INT, SEM_TYPE_INT) &&
              (!type_is_valid_compound_assignment(TOKEN_MODULO_ASSIGN, SEM_TYPE_FLOAT, SEM_TYPE_INT));
    assert_test(t20, "20. Assignment and compound assignment compatibility rules");

    /* Test 21: Return compatibility */
    int t21 = type_is_valid_return(SEM_TYPE_FLOAT, SEM_TYPE_INT) &&
              (!type_is_valid_return(SEM_TYPE_INT, SEM_TYPE_FLOAT)) &&
              type_is_valid_return(SEM_TYPE_VOID, SEM_TYPE_VOID) &&
              (!type_is_valid_return(SEM_TYPE_VOID, SEM_TYPE_INT));
    assert_test(t21, "21. Function return expression compatibility");

    /* Test 22: Array index compatibility */
    int t22 = type_is_valid_array_index(SEM_TYPE_INT) &&
              (!type_is_valid_array_index(SEM_TYPE_FLOAT)) &&
              (!type_is_valid_array_index(SEM_TYPE_CHAR));
    assert_test(t22, "22. Array index compatibility (int only; float/char rejected)");

    printf("\n====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
