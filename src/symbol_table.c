#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Fatal error: Out of memory copying string in symbol_table\n");
        exit(1);
    }
    strcpy(copy, str);
    return copy;
}

const char *symbol_kind_name(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_VARIABLE:            return "variable";
        case SYMBOL_ARRAY:               return "array";
        case SYMBOL_FUNCTION:            return "function";
        case SYMBOL_PARAMETER:           return "parameter";
        case SYMBOL_PREDEFINED_FUNCTION: return "predefined function";
        default:                         return "unknown";
    }
}

const char *scope_type_name(ScopeType type) {
    switch (type) {
        case SCOPE_GLOBAL:   return "Global";
        case SCOPE_FUNCTION: return "Function";
        case SCOPE_BLOCK:    return "Block";
        default:             return "Unknown";
    }
}

/*
 * Scope Lifecycle Management
 */
Scope *scope_create(ScopeType type, Scope *parent, const char *name, int level) {
    Scope *scope = (Scope *)malloc(sizeof(Scope));
    if (!scope) {
        fprintf(stderr, "Fatal error: Out of memory allocating Scope\n");
        exit(1);
    }

    scope->type = type;
    scope->level = level;
    scope->name = copy_string(name ? name : scope_type_name(type));
    scope->parent = parent;

    scope->symbol_capacity = 8;
    scope->symbol_count = 0;
    scope->symbols = (Symbol **)malloc(sizeof(Symbol *) * scope->symbol_capacity);
    if (!scope->symbols) {
        fprintf(stderr, "Fatal error: Out of memory allocating Scope symbols\n");
        exit(1);
    }

    scope->child_capacity = 4;
    scope->child_count = 0;
    scope->children = (Scope **)malloc(sizeof(Scope *) * scope->child_capacity);
    if (!scope->children) {
        fprintf(stderr, "Fatal error: Out of memory allocating Scope children\n");
        exit(1);
    }

    /* Register with parent scope if present */
    if (parent) {
        if (parent->child_count >= parent->child_capacity) {
            parent->child_capacity *= 2;
            parent->children = (Scope **)realloc(parent->children,
                sizeof(Scope *) * parent->child_capacity);
            if (!parent->children) {
                fprintf(stderr, "Fatal error: Out of memory reallocating parent children\n");
                exit(1);
            }
        }
        parent->children[parent->child_count++] = scope;
    }

    return scope;
}

void scope_destroy(Scope *scope) {
    if (!scope) return;

    /* Recursively destroy all child scopes */
    for (int i = 0; i < scope->child_count; i++) {
        scope_destroy(scope->children[i]);
    }
    free(scope->children);

    /* Free all symbols declared in this scope */
    for (int i = 0; i < scope->symbol_count; i++) {
        symbol_free(scope->symbols[i]);
    }
    free(scope->symbols);

    if (scope->name) {
        free(scope->name);
    }
    free(scope);
}

Scope *scope_enter(Scope *current, ScopeType type, const char *name) {
    int level = current ? current->level + 1 : 0;
    return scope_create(type, current, name, level);
}

Scope *scope_exit(Scope *current) {
    if (!current) return NULL;
    return current->parent;
}

/*
 * Symbol Creation Functions
 */
Symbol *symbol_create_variable(const char *name, ASTType type, int line, int column) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "Fatal error: Out of memory allocating Symbol\n");
        exit(1);
    }
    memset(sym, 0, sizeof(Symbol));
    sym->name = copy_string(name);
    sym->kind = SYMBOL_VARIABLE;
    sym->type = type;
    sym->return_type = type;
    sym->is_array = 0;
    sym->array_size = 0;
    sym->line = line;
    sym->column = column;
    return sym;
}

Symbol *symbol_create_array(const char *name, ASTType type, int size, int line, int column) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "Fatal error: Out of memory allocating Symbol\n");
        exit(1);
    }
    memset(sym, 0, sizeof(Symbol));
    sym->name = copy_string(name);
    sym->kind = SYMBOL_ARRAY;
    sym->type = type;
    sym->return_type = type;
    sym->is_array = 1;
    sym->array_size = size;
    sym->line = line;
    sym->column = column;
    return sym;
}

Symbol *symbol_create_parameter(const char *name, ASTType type, int line, int column) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "Fatal error: Out of memory allocating Symbol\n");
        exit(1);
    }
    memset(sym, 0, sizeof(Symbol));
    sym->name = copy_string(name);
    sym->kind = SYMBOL_PARAMETER;
    sym->type = type;
    sym->return_type = type;
    sym->is_array = 0;
    sym->array_size = 0;
    sym->line = line;
    sym->column = column;
    return sym;
}

Symbol *symbol_create_function(const char *name, ASTType return_type, int line, int column) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "Fatal error: Out of memory allocating Symbol\n");
        exit(1);
    }
    memset(sym, 0, sizeof(Symbol));
    sym->name = copy_string(name);
    sym->kind = SYMBOL_FUNCTION;
    sym->type = return_type;
    sym->return_type = return_type;
    sym->is_array = 0;
    sym->array_size = 0;
    sym->line = line;
    sym->column = column;
    sym->param_capacity = 4;
    sym->param_count = 0;
    sym->params = (SymbolParam *)malloc(sizeof(SymbolParam) * sym->param_capacity);
    if (!sym->params) {
        fprintf(stderr, "Fatal error: Out of memory allocating function params\n");
        exit(1);
    }
    return sym;
}

void symbol_function_add_param(Symbol *func_sym, const char *name, ASTType type) {
    if (!func_sym || (func_sym->kind != SYMBOL_FUNCTION && func_sym->kind != SYMBOL_PREDEFINED_FUNCTION)) {
        return;
    }
    if (func_sym->param_count >= func_sym->param_capacity) {
        func_sym->param_capacity *= 2;
        func_sym->params = (SymbolParam *)realloc(func_sym->params,
            sizeof(SymbolParam) * func_sym->param_capacity);
        if (!func_sym->params) {
            fprintf(stderr, "Fatal error: Out of memory reallocating function params\n");
            exit(1);
        }
    }
    func_sym->params[func_sym->param_count].name = copy_string(name);
    func_sym->params[func_sym->param_count].type = type;
    func_sym->param_count++;
}

Symbol *symbol_create_predefined(const char *name, ASTType return_type, int line, int column) {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "Fatal error: Out of memory allocating Symbol\n");
        exit(1);
    }
    memset(sym, 0, sizeof(Symbol));
    sym->name = copy_string(name);
    sym->kind = SYMBOL_PREDEFINED_FUNCTION;
    sym->type = return_type;
    sym->return_type = return_type;
    sym->is_array = 0;
    sym->array_size = 0;
    sym->line = line;
    sym->column = column;
    sym->param_capacity = 2;
    sym->param_count = 0;
    sym->params = (SymbolParam *)malloc(sizeof(SymbolParam) * sym->param_capacity);
    if (!sym->params) {
        fprintf(stderr, "Fatal error: Out of memory allocating predefined params\n");
        exit(1);
    }
    return sym;
}

void symbol_free(Symbol *symbol) {
    if (!symbol) return;
    if (symbol->name) {
        free(symbol->name);
    }
    if (symbol->params) {
        for (int i = 0; i < symbol->param_count; i++) {
            if (symbol->params[i].name) {
                free(symbol->params[i].name);
            }
        }
        free(symbol->params);
    }
    free(symbol);
}

/*
 * Scope Insertion and Lookup
 */
int scope_insert(Scope *scope, Symbol *symbol) {
    if (!scope || !symbol) return -1;

    /* Duplicate check strictly within the current scope */
    if (scope_lookup_current(scope, symbol->name) != NULL) {
        return -1; /* Duplicate declaration in same scope */
    }

    if (scope->symbol_count >= scope->symbol_capacity) {
        scope->symbol_capacity *= 2;
        scope->symbols = (Symbol **)realloc(scope->symbols,
            sizeof(Symbol *) * scope->symbol_capacity);
        if (!scope->symbols) {
            fprintf(stderr, "Fatal error: Out of memory reallocating Scope symbols\n");
            exit(1);
        }
    }

    symbol->scope = scope;
    scope->symbols[scope->symbol_count++] = symbol;
    return 0;
}

Symbol *scope_lookup_current(const Scope *scope, const char *name) {
    if (!scope || !name) return NULL;
    for (int i = 0; i < scope->symbol_count; i++) {
        if (scope->symbols[i]->name && strcmp(scope->symbols[i]->name, name) == 0) {
            return scope->symbols[i];
        }
    }
    return NULL;
}

Symbol *scope_lookup(const Scope *scope, const char *name) {
    if (!scope || !name) return NULL;
    const Scope *current = scope;
    while (current) {
        Symbol *sym = scope_lookup_current(current, name);
        if (sym) {
            return sym;
        }
        current = current->parent;
    }
    return NULL;
}

/*
 * Predefined environment initialization
 */
void scope_init_predefined_functions(Scope *global_scope) {
    if (!global_scope) return;

    /* printf: predefined function returning int */
    Symbol *printf_sym = symbol_create_predefined("printf", TYPE_INT, 0, 0);
    symbol_function_add_param(printf_sym, "format", TYPE_CHAR); /* format string pointer placeholder */
    scope_insert(global_scope, printf_sym);

    /* scanf: predefined function returning int */
    Symbol *scanf_sym = symbol_create_predefined("scanf", TYPE_INT, 0, 0);
    symbol_function_add_param(scanf_sym, "format", TYPE_CHAR); /* format string pointer placeholder */
    scope_insert(global_scope, scanf_sym);
}

/*
 * Visualization and pretty printing
 */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void scope_print(const Scope *scope, int indent) {
    if (!scope) return;

    print_indent(indent);
    printf("Scope [%s: '%s', level: %d, symbols: %d]:\n",
           scope_type_name(scope->type), scope->name, scope->level, scope->symbol_count);

    for (int i = 0; i < scope->symbol_count; i++) {
        const Symbol *sym = scope->symbols[i];
        print_indent(indent + 1);

        if (sym->kind == SYMBOL_VARIABLE) {
            printf("- variable %s : %s (line: %d, col: %d)\n",
                   sym->name, ast_type_name(sym->type), sym->line, sym->column);
        } else if (sym->kind == SYMBOL_ARRAY) {
            printf("- array %s[%d] : %s (line: %d, col: %d)\n",
                   sym->name, sym->array_size, ast_type_name(sym->type), sym->line, sym->column);
        } else if (sym->kind == SYMBOL_PARAMETER) {
            printf("- parameter %s : %s (line: %d, col: %d)\n",
                   sym->name, ast_type_name(sym->type), sym->line, sym->column);
        } else if (sym->kind == SYMBOL_FUNCTION || sym->kind == SYMBOL_PREDEFINED_FUNCTION) {
            printf("- %s %s(", symbol_kind_name(sym->kind), sym->name);
            for (int p = 0; p < sym->param_count; p++) {
                printf("%s%s: %s", (p > 0 ? ", " : ""),
                       sym->params[p].name ? sym->params[p].name : "param",
                       ast_type_name(sym->params[p].type));
            }
            printf(") -> %s (line: %d, col: %d)\n",
                   ast_type_name(sym->return_type), sym->line, sym->column);
        }
    }

    /* Print child scopes */
    for (int c = 0; c < scope->child_count; c++) {
        scope_print(scope->children[c], indent + 1);
    }
}
