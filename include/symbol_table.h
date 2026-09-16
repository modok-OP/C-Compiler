#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

/*
 * Symbol Kind enumeration matching Section 25 of the frozen specification.
 */
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_ARRAY,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_PREDEFINED_FUNCTION
} SymbolKind;

/*
 * Function parameter representation within a function symbol.
 */
typedef struct {
    char *name;
    ASTType type;
} SymbolParam;

/*
 * Forward declaration of Scope structure.
 */
struct Scope;

/*
 * Symbol entry structure containing all metadata needed for semantic validation.
 */
typedef struct Symbol {
    char *name;
    SymbolKind kind;
    ASTType type;               /* Type of variable/param, element type of array, or return type */
    struct Scope *scope;        /* Enclosing scope reference */
    int is_array;               /* 1 if array, 0 otherwise */
    int array_size;             /* Array dimension */

    /* Function metadata */
    ASTType return_type;
    SymbolParam *params;
    int param_count;
    int param_capacity;

    /* Source coordinates for diagnostics */
    int line;
    int column;
} Symbol;

/*
 * Scope categories: global/program, function, and nested block scopes.
 */
typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK
} ScopeType;

/*
 * Scope structure maintaining local symbols, enclosing parent, and child scopes.
 */
typedef struct Scope {
    ScopeType type;
    int level;                  /* 0: global, 1: function, 2+: nested blocks */
    char *name;                 /* Descriptive identifier ("global", function name, or "block") */
    struct Scope *parent;       /* Enclosing parent scope (NULL for global) */

    /* Symbols declared directly in this scope */
    Symbol **symbols;
    int symbol_count;
    int symbol_capacity;

    /* Child scopes for hierarchical ownership and traversal */
    struct Scope **children;
    int child_count;
    int child_capacity;
} Scope;

/*
 * Scope lifecycle management
 */
Scope *scope_create(ScopeType type, Scope *parent, const char *name, int level);
void scope_destroy(Scope *scope);
Scope *scope_enter(Scope *current, ScopeType type, const char *name);
Scope *scope_exit(Scope *current);

/*
 * Symbol creation functions
 */
Symbol *symbol_create_variable(const char *name, ASTType type, int line, int column);
Symbol *symbol_create_array(const char *name, ASTType type, int size, int line, int column);
Symbol *symbol_create_parameter(const char *name, ASTType type, int line, int column);
Symbol *symbol_create_function(const char *name, ASTType return_type, int line, int column);
void symbol_function_add_param(Symbol *func_sym, const char *name, ASTType type);
Symbol *symbol_create_predefined(const char *name, ASTType return_type, int line, int column);
void symbol_free(Symbol *symbol);

/*
 * Scope insertion and lookup
 */
int scope_insert(Scope *scope, Symbol *symbol);
Symbol *scope_lookup_current(const Scope *scope, const char *name);
Symbol *scope_lookup(const Scope *scope, const char *name);

/*
 * Predefined environment initialization
 */
void scope_init_predefined_functions(Scope *global_scope);

/*
 * Inspection and visualization helpers
 */
const char *symbol_kind_name(SymbolKind kind);
const char *scope_type_name(ScopeType type);
void scope_print(const Scope *scope, int indent);

#endif /* SYMBOL_TABLE_H */
