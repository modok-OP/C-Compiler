#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"
#include "type_system.h"

/*
 * Semantic analysis context managing scope hierarchy, active scope,
 * control-flow context, and diagnostic counts.
 */
typedef struct {
    Scope *global_scope;
    Scope *current_scope;
    Symbol *current_function;    /* Enclosing function symbol */
    int loop_depth;              /* Current loop nesting depth (for break/continue) */
    int function_has_return;     /* 1 if valid return encountered in non-void function */
    int main_count;              /* Count of main function definitions */
    int error_count;
    int warning_count;
} SemanticContext;

/*
 * Semantic context lifecycle
 */
SemanticContext *semantic_context_create(void);
void semantic_context_destroy(SemanticContext *ctx);

/*
 * Scope building pass (Stage 7)
 */
int semantic_build_symbols(SemanticContext *ctx, const ASTNode *program);

/*
 * Full Semantic Analysis entry point (Stage 9)
 * Analyzes declarations, types, expressions, control flow, functions, and main().
 * Returns 0 on success, or number of semantic errors found.
 */
int semantic_analyze(SemanticContext *ctx, const ASTNode *program);

/*
 * Expression type inference helper
 */
SemType semantic_check_expression(SemanticContext *ctx, const ASTNode *expr);

#endif /* SEMANTIC_H */
