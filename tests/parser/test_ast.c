#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

/*
 * Test program to verify AST construction, recursive printing,
 * and memory deallocation.
 *
 * Constructs AST corresponding to:
 * int main() {
 *     int x = 10;
 *     x = x + 5;
 *     return x;
 * }
 */
int main(void) {
    printf("========================================\n");
    printf("        AST Structure Verification\n");
    printf("========================================\n\n");

    /* 1. Program root */
    ASTNode *prog = ast_create_program(1, 1);

    /* 2. Function: int main() */
    ASTNode *func = ast_create_function("main", TYPE_INT, 1, 1);

    /* 3. Function body: Block */
    ASTNode *block = ast_create_block(1, 12);

    /* 4. Statement 1: int x = 10; */
    ASTNode *lit10 = ast_create_literal_int(10, "10", 2, 13);
    ASTNode *decl = ast_create_declaration(TYPE_INT, "x", lit10, 0, 0, 2, 5);
    ast_block_add_stmt(block, decl);

    /* 5. Statement 2: x = x + 5; */
    ASTNode *id_left = ast_create_identifier("x", 3, 5);
    ASTNode *id_x = ast_create_identifier("x", 3, 9);
    ASTNode *lit5 = ast_create_literal_int(5, "5", 3, 13);
    ASTNode *bin_add = ast_create_binary(TOKEN_PLUS, id_x, lit5, 3, 11);
    ASTNode *assign = ast_create_assignment(TOKEN_ASSIGN, id_left, bin_add, 3, 7);
    ast_block_add_stmt(block, assign);

    /* 6. Statement 3: return x; */
    ASTNode *id_ret = ast_create_identifier("x", 4, 12);
    ASTNode *ret = ast_create_return(id_ret, 4, 5);
    ast_block_add_stmt(block, ret);

    /* 7. Assemble hierarchy */
    ast_function_set_body(func, block);
    ast_program_add_decl(prog, func);

    /* 8. Print AST */
    printf("--- Constructed AST Tree ---\n");
    ast_print(prog, 0);
    printf("--- End of AST Tree ---\n\n");

    /* 9. Deallocate AST */
    ast_free(prog);
    printf("AST memory deallocated cleanly.\n");

    return 0;
}
