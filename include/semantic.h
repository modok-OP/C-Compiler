#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

/*
 * Semantic analysis context managing scope hierarchy, active scope, and diagnostics.
 */
typedef struct {
    Scope *global_scope;
    Scope *current_scope;
    int error_count;
    int warning_count;
} SemanticContext;

/*
 * Semantic context lifecycle
 */
SemanticContext *semantic_context_create(void);
void semantic_context_destroy(SemanticContext *ctx);

/*
 * Stage 7 Entry Point:
 * Traverses the AST, creates nested scopes, and populates the symbol table.
 * Detects duplicate declarations in the same scope (SEM-02).
 * Returns 0 on success, or number of errors encountered.
 */
int semantic_build_symbols(SemanticContext *ctx, const ASTNode *program);

#endif /* SEMANTIC_H */
