#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
 * Diagnostic error reporting helper
 */
static void semantic_error(SemanticContext *ctx, int line, int col, const char *fmt, ...) {
    ctx->error_count++;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Semantic Error [%d:%d]: ", line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

SemanticContext *semantic_context_create(void) {
    SemanticContext *ctx = (SemanticContext *)malloc(sizeof(SemanticContext));
    if (!ctx) {
        fprintf(stderr, "Fatal error: Out of memory allocating SemanticContext\n");
        exit(1);
    }
    ctx->global_scope = scope_create(SCOPE_GLOBAL, NULL, "global", 0);
    scope_init_predefined_functions(ctx->global_scope);
    ctx->current_scope = ctx->global_scope;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    return ctx;
}

void semantic_context_destroy(SemanticContext *ctx) {
    if (!ctx) return;
    if (ctx->global_scope) {
        scope_destroy(ctx->global_scope);
    }
    free(ctx);
}

/*
 * Traverses statement nodes within a scope, managing nested block scopes.
 */
static void traverse_statement(SemanticContext *ctx, const ASTNode *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case AST_DECLARATION: {
            const char *name = stmt->data.declaration.name;
            if (scope_lookup_current(ctx->current_scope, name) != NULL) {
                semantic_error(ctx, stmt->line, stmt->column,
                               "SEM-02: Duplicate declaration of '%s'", name);
            } else {
                Symbol *sym = NULL;
                if (stmt->data.declaration.is_array) {
                    sym = symbol_create_array(name,
                                              stmt->data.declaration.type,
                                              stmt->data.declaration.array_size,
                                              stmt->line, stmt->column);
                } else {
                    sym = symbol_create_variable(name,
                                                 stmt->data.declaration.type,
                                                 stmt->line, stmt->column);
                }
                scope_insert(ctx->current_scope, sym);
            }
            break;
        }

        case AST_BLOCK: {
            /* Nested block introduces a new block scope */
            ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_BLOCK, "block");
            for (int i = 0; i < stmt->data.block.stmt_count; i++) {
                traverse_statement(ctx, stmt->data.block.statements[i]);
            }
            ctx->current_scope = scope_exit(ctx->current_scope);
            break;
        }

        case AST_IF:
            traverse_statement(ctx, stmt->data.if_stmt.then_branch);
            if (stmt->data.if_stmt.else_branch) {
                traverse_statement(ctx, stmt->data.if_stmt.else_branch);
            }
            break;

        case AST_WHILE:
        case AST_DO_WHILE:
            traverse_statement(ctx, stmt->data.while_stmt.body);
            break;

        case AST_FOR:
            /* If for-init declares a loop variable, enter iteration scope */
            if (stmt->data.for_stmt.init && stmt->data.for_stmt.init->kind == AST_DECLARATION) {
                ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_BLOCK, "for");
                traverse_statement(ctx, stmt->data.for_stmt.init);
                traverse_statement(ctx, stmt->data.for_stmt.body);
                ctx->current_scope = scope_exit(ctx->current_scope);
            } else {
                traverse_statement(ctx, stmt->data.for_stmt.body);
            }
            break;

        default:
            /* Non-declaration statements (expressions, return, break, continue) do not affect scopes */
            break;
    }
}

/*
 * Traverses top-level external declarations (global variables, arrays, and functions).
 */
int semantic_build_symbols(SemanticContext *ctx, const ASTNode *program) {
    if (!ctx || !program || program->kind != AST_PROGRAM) {
        return -1;
    }

    ctx->current_scope = ctx->global_scope;

    for (int i = 0; i < program->data.program.decl_count; i++) {
        const ASTNode *decl = program->data.program.declarations[i];
        if (!decl) continue;

        if (decl->kind == AST_DECLARATION) {
            /* Global variable or 1-D array */
            const char *name = decl->data.declaration.name;
            if (scope_lookup_current(ctx->global_scope, name) != NULL) {
                semantic_error(ctx, decl->line, decl->column,
                               "SEM-02: Duplicate declaration of '%s'", name);
            } else {
                Symbol *sym = NULL;
                if (decl->data.declaration.is_array) {
                    sym = symbol_create_array(name,
                                              decl->data.declaration.type,
                                              decl->data.declaration.array_size,
                                              decl->line, decl->column);
                } else {
                    sym = symbol_create_variable(name,
                                                 decl->data.declaration.type,
                                                 decl->line, decl->column);
                }
                scope_insert(ctx->global_scope, sym);
            }
        } else if (decl->kind == AST_FUNCTION) {
            /* Function definition */
            const char *func_name = decl->data.function.name;

            /* Check duplicate function name in global namespace */
            if (scope_lookup_current(ctx->global_scope, func_name) != NULL) {
                semantic_error(ctx, decl->line, decl->column,
                               "SEM-02: Duplicate declaration of '%s'", func_name);
            } else {
                Symbol *func_sym = symbol_create_function(func_name,
                                                          decl->data.function.return_type,
                                                          decl->line, decl->column);
                for (int p = 0; p < decl->data.function.param_count; p++) {
                    symbol_function_add_param(func_sym,
                                              decl->data.function.params[p].name,
                                              decl->data.function.params[p].type);
                }
                scope_insert(ctx->global_scope, func_sym);
            }

            /* Enter function scope */
            ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_FUNCTION, func_name);

            /* Insert parameters into function scope */
            for (int p = 0; p < decl->data.function.param_count; p++) {
                const char *pname = decl->data.function.params[p].name;
                ASTType ptype = decl->data.function.params[p].type;
                if (scope_lookup_current(ctx->current_scope, pname) != NULL) {
                    semantic_error(ctx, decl->line, decl->column,
                                   "SEM-02: Duplicate declaration of '%s'", pname);
                } else {
                    Symbol *psym = symbol_create_parameter(pname, ptype, decl->line, decl->column);
                    scope_insert(ctx->current_scope, psym);
                }
            }

            /*
             * Traverse top-level body statements directly within function scope
             */
            if (decl->data.function.body && decl->data.function.body->kind == AST_BLOCK) {
                const ASTNode *body = decl->data.function.body;
                for (int s = 0; s < body->data.block.stmt_count; s++) {
                    traverse_statement(ctx, body->data.block.statements[s]);
                }
            }

            /* Exit function scope back to global scope */
            ctx->current_scope = scope_exit(ctx->current_scope);
        }
    }

    return ctx->error_count;
}
