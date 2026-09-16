#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symbol_table.h"

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
    printf("#       STAGE 7: SYMBOL TABLE UNIT TEST SUITE      #\n");
    printf("####################################################\n\n");

    Scope *global_scope = scope_create(SCOPE_GLOBAL, NULL, "global", 0);

    /* Test 1: Global variable insertion */
    Symbol *gvar = symbol_create_variable("g_count", TYPE_INT, 1, 1);
    int res1 = scope_insert(global_scope, gvar);
    Symbol *found1 = scope_lookup(global_scope, "g_count");
    assert_test(res1 == 0 && found1 != NULL && found1->type == TYPE_INT,
                "1. Global variable insertion");

    /* Test 2: Local variable insertion in nested block */
    Scope *func_scope = scope_enter(global_scope, SCOPE_FUNCTION, "test_func");
    Symbol *lvar = symbol_create_variable("local_x", TYPE_FLOAT, 5, 5);
    int res2 = scope_insert(func_scope, lvar);
    Symbol *found2 = scope_lookup_current(func_scope, "local_x");
    assert_test(res2 == 0 && found2 != NULL && found2->type == TYPE_FLOAT,
                "2. Local variable insertion");

    /* Test 3: Duplicate declaration in same scope */
    Symbol *dup_var = symbol_create_variable("local_x", TYPE_INT, 6, 5);
    int res3 = scope_insert(func_scope, dup_var);
    assert_test(res3 == -1, "3. Duplicate declaration in same scope rejected");
    symbol_free(dup_var); /* Free rejected symbol */

    /* Test 4: Legal inner shadowing */
    Scope *block_scope = scope_enter(func_scope, SCOPE_BLOCK, "inner_block");
    Symbol *shadow_var = symbol_create_variable("local_x", TYPE_CHAR, 10, 9);
    int res4 = scope_insert(block_scope, shadow_var);
    Symbol *found_shadow = scope_lookup_current(block_scope, "local_x");
    assert_test(res4 == 0 && found_shadow != NULL && found_shadow->type == TYPE_CHAR,
                "4. Legal inner shadowing of outer variable");

    /* Test 5: Lookup through parent scopes */
    Symbol *found_parent_g = scope_lookup(block_scope, "g_count");
    assert_test(found_parent_g != NULL && found_parent_g == gvar,
                "5. Lookup through parent scopes up to global");

    /* Test 6: Hidden inner variable after leaving scope */
    Symbol *block_only_var = symbol_create_variable("temp_val", TYPE_INT, 11, 9);
    scope_insert(block_scope, block_only_var);
    Scope *exited_scope = scope_exit(block_scope); /* Back to func_scope */
    Symbol *lookup_exited = scope_lookup_current(exited_scope, "temp_val");
    assert_test(lookup_exited == NULL,
                "6. Hidden inner variable after leaving scope");

    /* Test 7: Function insertion */
    Symbol *func_sym = symbol_create_function("compute", TYPE_FLOAT, 15, 1);
    symbol_function_add_param(func_sym, "base", TYPE_FLOAT);
    symbol_function_add_param(func_sym, "exp", TYPE_INT);
    int res7 = scope_insert(global_scope, func_sym);
    Symbol *found_func = scope_lookup(global_scope, "compute");
    assert_test(res7 == 0 && found_func != NULL && found_func->kind == SYMBOL_FUNCTION &&
                found_func->return_type == TYPE_FLOAT && found_func->param_count == 2,
                "7. Function insertion with return type and parameter list");

    /* Test 8: Parameter insertion */
    Symbol *param_sym = symbol_create_parameter("arg1", TYPE_INT, 20, 10);
    int res8 = scope_insert(func_scope, param_sym);
    Symbol *found_param = scope_lookup_current(func_scope, "arg1");
    assert_test(res8 == 0 && found_param != NULL && found_param->kind == SYMBOL_PARAMETER &&
                found_param->type == TYPE_INT,
                "8. Parameter insertion into function scope");

    /* Test 9: Array metadata */
    Symbol *arr_sym = symbol_create_array("buffer", TYPE_CHAR, 256, 25, 5);
    int res9 = scope_insert(global_scope, arr_sym);
    Symbol *found_arr = scope_lookup(global_scope, "buffer");
    assert_test(res9 == 0 && found_arr != NULL && found_arr->kind == SYMBOL_ARRAY &&
                found_arr->is_array == 1 && found_arr->array_size == 256 &&
                found_arr->type == TYPE_CHAR,
                "9. 1-D Array symbol metadata and size preservation");

    /* Test 10: Predefined printf */
    scope_init_predefined_functions(global_scope);
    Symbol *found_printf = scope_lookup(global_scope, "printf");
    assert_test(found_printf != NULL && found_printf->kind == SYMBOL_PREDEFINED_FUNCTION &&
                strcmp(found_printf->name, "printf") == 0,
                "10. Predefined printf present in initial environment");

    /* Test 11: Predefined scanf */
    Symbol *found_scanf = scope_lookup(global_scope, "scanf");
    assert_test(found_scanf != NULL && found_scanf->kind == SYMBOL_PREDEFINED_FUNCTION &&
                strcmp(found_scanf->name, "scanf") == 0,
                "11. Predefined scanf present in initial environment");

    printf("\n--- Scope Hierarchy Visualization ---\n");
    scope_print(global_scope, 0);

    /* Clean memory */
    scope_destroy(global_scope);

    printf("\n====================================================\n");
    printf("TEST SUMMARY: %d / %d tests passed (%.1f%%)\n",
           passed_tests, total_tests, (float)passed_tests / total_tests * 100.0f);
    printf("====================================================\n");

    return (passed_tests == total_tests) ? 0 : 1;
}
