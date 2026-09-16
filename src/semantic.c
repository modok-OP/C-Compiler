#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
 * Semantic diagnostic reporter
 */
static void semantic_error(SemanticContext *ctx, int line, int col, const char *code, const char *fmt, ...) {
    ctx->error_count++;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Semantic Error [%d:%d]: %s: ", line, col, code);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/*
 * Checks if an identifier was ever declared in any scope in the entire scope tree.
 * Used to distinguish SEM-03 (out of scope) from SEM-01 (never declared).
 */
static int symbol_exists_in_any_scope(const Scope *scope, const char *name) {
    if (!scope || !name) return 0;
    if (scope_lookup_current(scope, name) != NULL) {
        return 1;
    }
    for (int i = 0; i < scope->child_count; i++) {
        if (symbol_exists_in_any_scope(scope->children[i], name)) {
            return 1;
        }
    }
    return 0;
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
    ctx->current_function = NULL;
    ctx->loop_depth = 0;
    ctx->function_has_return = 0;
    ctx->main_count = 0;
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

/* Forward declarations */
static void validate_statement(SemanticContext *ctx, const ASTNode *stmt);
SemType semantic_check_expression(SemanticContext *ctx, const ASTNode *expr);

/*
 * =========================================================================
 * Stage 10: Printf / Scanf Semantic Checking
 * =========================================================================
 */

typedef struct {
    char spec;      /* 'd', 'f', 'c', 's', or '\0' if unsupported */
    char raw_char;  /* character after '%' */
    int is_valid;   /* 1 if valid supported specifier, 0 if unsupported */
} FormatSpec;

#define MAX_FORMAT_SPECS 128

static int parse_format_string(SemanticContext *ctx, const ASTNode *fmt_node, int is_scanf,
                               FormatSpec *specs, int max_specs, int *out_count) {
    *out_count = 0;
    if (!fmt_node) return 0;

    if (fmt_node->kind != AST_LITERAL || fmt_node->data.literal.literal_type != LITERAL_STRING) {
        semantic_error(ctx, fmt_node->line, fmt_node->column, is_scanf ? "SEM-18" : "SEM-17",
                       "%s requires string literal format as first argument",
                       is_scanf ? "scanf" : "printf");
        return -1;
    }

    const char *str = fmt_node->data.literal.val.string_val;
    if (!str) str = "";

    int count = 0;
    int has_error = 0;
    int i = 0;

    while (str[i] != '\0') {
        if (str[i] == '%') {
            i++;
            if (str[i] == '%') {
                /* Escaped literal percent sign, takes no argument */
                i++;
                continue;
            }
            if (str[i] == '\0') {
                semantic_error(ctx, fmt_node->line, fmt_node->column, is_scanf ? "SEM-18" : "SEM-17",
                               "Incomplete format specifier at end of %s format string",
                               is_scanf ? "scanf" : "printf");
                has_error = 1;
                break;
            }

            char c = str[i];
            int valid = 0;

            if (!is_scanf) {
                /* printf supports %d, %f, %c, %s */
                if (c == 'd' || c == 'f' || c == 'c' || c == 's') {
                    valid = 1;
                }
            } else {
                /* scanf supports %d, %f, %c (%s is NOT supported) */
                if (c == 'd' || c == 'f' || c == 'c') {
                    valid = 1;
                }
            }

            if (!valid) {
                semantic_error(ctx, fmt_node->line, fmt_node->column, is_scanf ? "SEM-18" : "SEM-17",
                               "Unsupported %s format specifier '%%%c'",
                               is_scanf ? "scanf" : "printf", c);
                has_error = 1;
            }

            if (count < max_specs) {
                specs[count].spec = valid ? c : '\0';
                specs[count].raw_char = c;
                specs[count].is_valid = valid;
                count++;
            }
            i++;
        } else {
            i++;
        }
    }

    *out_count = count;
    return has_error ? 1 : 0;
}

static SemType validate_printf_call(SemanticContext *ctx, const ASTNode *call) {
    int arg_count = call->data.call.arg_count;
    ASTNode **args = call->data.call.args;

    if (arg_count < 1) {
        semantic_error(ctx, call->line, call->column, "SEM-17",
                       "printf requires at least one argument (format string)");
        return SEM_TYPE_INT;
    }

    FormatSpec specs[MAX_FORMAT_SPECS];
    int spec_count = 0;
    int parse_res = parse_format_string(ctx, args[0], 0, specs, MAX_FORMAT_SPECS, &spec_count);

    if (parse_res == -1) {
        /* First arg is not string literal. Evaluate all arguments to resolve symbols. */
        for (int i = 0; i < arg_count; i++) {
            semantic_check_expression(ctx, args[i]);
        }
        return SEM_TYPE_INT;
    }

    int passed_args = arg_count - 1;

    /* Check argument count */
    if (passed_args != spec_count) {
        semantic_error(ctx, call->line, call->column, "SEM-17",
                       "printf format requires %d argument(s), got %d",
                       spec_count, passed_args);
    }

    /* Check argument types */
    int check_count = (passed_args < spec_count) ? passed_args : spec_count;
    for (int i = 0; i < check_count; i++) {
        ASTNode *arg = args[i + 1];
        SemType arg_t = semantic_check_expression(ctx, arg);

        if (!specs[i].is_valid) {
            /* Unsupported specifier error was already reported */
            continue;
        }

        if (arg_t == SEM_TYPE_ERROR) {
            continue;
        }

        switch (specs[i].spec) {
            case 'd':
                if (arg_t != SEM_TYPE_INT) {
                    semantic_error(ctx, arg->line, arg->column, "SEM-17",
                                   "%%d requires int argument");
                }
                break;
            case 'f':
                if (arg_t != SEM_TYPE_FLOAT) {
                    semantic_error(ctx, arg->line, arg->column, "SEM-17",
                                   "%%f requires float argument");
                }
                break;
            case 'c':
                if (arg_t != SEM_TYPE_CHAR) {
                    semantic_error(ctx, arg->line, arg->column, "SEM-17",
                                   "%%c requires char argument");
                }
                break;
            case 's':
                if (arg->kind != AST_LITERAL || arg->data.literal.literal_type != LITERAL_STRING || arg_t != SEM_TYPE_STRING) {
                    semantic_error(ctx, arg->line, arg->column, "SEM-17",
                                   "%%s requires string literal argument");
                }
                break;
        }
    }

    /* Evaluate any extra arguments passed beyond spec_count to resolve symbols */
    for (int i = check_count; i < passed_args; i++) {
        semantic_check_expression(ctx, args[i + 1]);
    }

    return SEM_TYPE_INT;
}

static SemType validate_scanf_call(SemanticContext *ctx, const ASTNode *call) {
    int arg_count = call->data.call.arg_count;
    ASTNode **args = call->data.call.args;

    if (arg_count < 1) {
        semantic_error(ctx, call->line, call->column, "SEM-18",
                       "scanf requires at least one argument (format string)");
        return SEM_TYPE_INT;
    }

    FormatSpec specs[MAX_FORMAT_SPECS];
    int spec_count = 0;
    int parse_res = parse_format_string(ctx, args[0], 1, specs, MAX_FORMAT_SPECS, &spec_count);

    if (parse_res == -1) {
        /* First arg is not string literal. Evaluate all arguments to resolve symbols. */
        for (int i = 0; i < arg_count; i++) {
            semantic_check_expression(ctx, args[i]);
        }
        return SEM_TYPE_INT;
    }

    int passed_args = arg_count - 1;

    /* Check destination count */
    if (passed_args != spec_count) {
        semantic_error(ctx, call->line, call->column, "SEM-18",
                       "scanf format requires %d destination(s), got %d",
                       spec_count, passed_args);
    }

    /* Validate each passed destination */
    int check_count = (passed_args < spec_count) ? passed_args : spec_count;
    for (int i = 0; i < check_count; i++) {
        ASTNode *dest = args[i + 1];

        /* Destination MUST be &identifier */
        if (dest->kind != AST_UNARY_EXPR || dest->data.unary_expr.op != TOKEN_AMPERSAND) {
            semantic_error(ctx, dest->line, dest->column, "SEM-18",
                           "scanf destination must be in the form &identifier");
            semantic_check_expression(ctx, dest);
            continue;
        }

        ASTNode *id_node = dest->data.unary_expr.operand;
        if (!id_node || id_node->kind != AST_IDENTIFIER) {
            semantic_error(ctx, dest->line, dest->column, "SEM-18",
                           "scanf destination must be in the form &identifier");
            if (id_node) {
                semantic_check_expression(ctx, id_node);
            }
            continue;
        }

        const char *var_name = id_node->data.identifier.name;
        Symbol *var_sym = scope_lookup(ctx->current_scope, var_name);
        if (!var_sym) {
            if (symbol_exists_in_any_scope(ctx->global_scope, var_name)) {
                semantic_error(ctx, id_node->line, id_node->column, "SEM-03",
                               "Identifier '%s' is out of scope", var_name);
            } else {
                semantic_error(ctx, id_node->line, id_node->column, "SEM-01",
                               "Identifier '%s' is not declared", var_name);
            }
            continue;
        }

        if (var_sym->kind != SYMBOL_VARIABLE && var_sym->kind != SYMBOL_PARAMETER) {
            semantic_error(ctx, id_node->line, id_node->column, "SEM-18",
                           "scanf destination '%s' must be a variable", var_name);
            continue;
        }

        if (var_sym->is_array) {
            semantic_error(ctx, id_node->line, id_node->column, "SEM-18",
                           "scanf destination '%s' cannot be an array", var_name);
            continue;
        }

        if (!specs[i].is_valid) {
            /* Unsupported specifier error was already reported */
            continue;
        }

        switch (specs[i].spec) {
            case 'd':
                if (var_sym->type != TYPE_INT) {
                    semantic_error(ctx, dest->line, dest->column, "SEM-18",
                                   "%%d requires int destination");
                }
                break;
            case 'f':
                if (var_sym->type != TYPE_FLOAT) {
                    semantic_error(ctx, dest->line, dest->column, "SEM-18",
                                   "%%f requires float destination");
                }
                break;
            case 'c':
                if (var_sym->type != TYPE_CHAR) {
                    semantic_error(ctx, dest->line, dest->column, "SEM-18",
                                   "%%c requires char destination");
                }
                break;
        }
    }

    /* Evaluate any extra destination arguments beyond spec_count */
    for (int i = check_count; i < passed_args; i++) {
        ASTNode *dest = args[i + 1];
        if (dest->kind != AST_UNARY_EXPR || dest->data.unary_expr.op != TOKEN_AMPERSAND) {
            semantic_error(ctx, dest->line, dest->column, "SEM-18",
                           "scanf destination must be in the form &identifier");
            semantic_check_expression(ctx, dest);
        } else {
            ASTNode *id_node = dest->data.unary_expr.operand;
            if (!id_node || id_node->kind != AST_IDENTIFIER) {
                semantic_error(ctx, dest->line, dest->column, "SEM-18",
                               "scanf destination must be in the form &identifier");
                if (id_node) semantic_check_expression(ctx, id_node);
            } else {
                const char *var_name = id_node->data.identifier.name;
                Symbol *var_sym = scope_lookup(ctx->current_scope, var_name);
                if (!var_sym) {
                    if (symbol_exists_in_any_scope(ctx->global_scope, var_name)) {
                        semantic_error(ctx, id_node->line, id_node->column, "SEM-03",
                                       "Identifier '%s' is out of scope", var_name);
                    } else {
                        semantic_error(ctx, id_node->line, id_node->column, "SEM-01",
                                       "Identifier '%s' is not declared", var_name);
                    }
                }
            }
        }
    }

    return SEM_TYPE_INT;
}


/*
 * Expression Type Inference Visitor
 */
SemType semantic_check_expression(SemanticContext *ctx, const ASTNode *expr) {
    if (!expr) return SEM_TYPE_ERROR;

    switch (expr->kind) {
        case AST_LITERAL: {
            switch (expr->data.literal.literal_type) {
                case LITERAL_INT:    return SEM_TYPE_INT;
                case LITERAL_FLOAT:  return SEM_TYPE_FLOAT;
                case LITERAL_CHAR:   return SEM_TYPE_CHAR;
                case LITERAL_STRING: return SEM_TYPE_STRING;
                default:             return SEM_TYPE_ERROR;
            }
        }

        case AST_IDENTIFIER: {
            const char *name = expr->data.identifier.name;
            Symbol *sym = scope_lookup(ctx->current_scope, name);
            if (!sym) {
                if (symbol_exists_in_any_scope(ctx->global_scope, name)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-03",
                                   "Identifier '%s' is out of scope", name);
                } else {
                    semantic_error(ctx, expr->line, expr->column, "SEM-01",
                                   "Identifier '%s' is not declared", name);
                }
                return SEM_TYPE_ERROR;
            }

            if (sym->kind == SYMBOL_FUNCTION || sym->kind == SYMBOL_PREDEFINED_FUNCTION) {
                semantic_error(ctx, expr->line, expr->column, "SEM-01",
                               "Function '%s' cannot be used as an expression value", name);
                return SEM_TYPE_ERROR;
            }

            return sem_type_from_ast_type(sym->type);
        }

        case AST_ARRAY_ACCESS: {
            const ASTNode *arr = expr->data.array_access.array_expr;
            Symbol *sym = NULL;
            if (arr && arr->kind == AST_IDENTIFIER) {
                const char *arr_name = arr->data.identifier.name;
                sym = scope_lookup(ctx->current_scope, arr_name);
                if (!sym) {
                    if (symbol_exists_in_any_scope(ctx->global_scope, arr_name)) {
                        semantic_error(ctx, arr->line, arr->column, "SEM-03",
                                       "Identifier '%s' is out of scope", arr_name);
                    } else {
                        semantic_error(ctx, arr->line, arr->column, "SEM-01",
                                       "Identifier '%s' is not declared", arr_name);
                    }
                } else if (sym->kind != SYMBOL_ARRAY && !sym->is_array) {
                    semantic_error(ctx, arr->line, arr->column, "SEM-15",
                                   "Identifier '%s' is not an array", arr_name);
                }
            } else {
                semantic_error(ctx, expr->line, expr->column, "SEM-15",
                               "Invalid array access expression");
            }

            SemType idx_type = semantic_check_expression(ctx, expr->data.array_access.index_expr);
            if (idx_type != SEM_TYPE_ERROR && !type_is_valid_array_index(idx_type)) {
                semantic_error(ctx, expr->line, expr->column, "SEM-15",
                               "Array index must be int");
            }

            if (sym) {
                return sem_type_from_ast_type(sym->type);
            }
            return SEM_TYPE_ERROR;
        }

        case AST_BINARY_EXPR: {
            TokenType op = expr->data.binary_expr.op;
            SemType left = semantic_check_expression(ctx, expr->data.binary_expr.left);
            SemType right = semantic_check_expression(ctx, expr->data.binary_expr.right);

            if (left == SEM_TYPE_ERROR || right == SEM_TYPE_ERROR) {
                return SEM_TYPE_ERROR;
            }

            if (op == TOKEN_MODULO) {
                if (!type_is_valid_modulus(left, right)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-06",
                                   "Modulus requires int operands");
                    return SEM_TYPE_ERROR;
                }
                return SEM_TYPE_INT;
            }

            if (op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_MULTIPLY || op == TOKEN_DIVIDE) {
                SemType res = type_arithmetic_result(op, left, right);
                if (res == SEM_TYPE_ERROR) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-05",
                                   "Invalid operand types for arithmetic");
                    return SEM_TYPE_ERROR;
                }
                return res;
            }

            if (op == TOKEN_LESS || op == TOKEN_LESS_EQUAL ||
                op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL ||
                op == TOKEN_EQUAL || op == TOKEN_NOT_EQUAL) {
                if (!type_is_valid_comparison(left, right)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-05",
                                   "Invalid operand types for comparison");
                    return SEM_TYPE_ERROR;
                }
                return SEM_TYPE_BOOL;
            }

            if (op == TOKEN_LOGICAL_AND || op == TOKEN_LOGICAL_OR) {
                if (!type_is_valid_logical_operand(left) || !type_is_valid_logical_operand(right)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-08",
                                   "Logical operator requires boolean operands");
                    return SEM_TYPE_ERROR;
                }
                return SEM_TYPE_BOOL;
            }

            return SEM_TYPE_ERROR;
        }

        case AST_UNARY_EXPR: {
            TokenType op = expr->data.unary_expr.op;
            const ASTNode *operand = expr->data.unary_expr.operand;

            if (op == TOKEN_LOGICAL_NOT) {
                SemType t = semantic_check_expression(ctx, operand);
                if (t != SEM_TYPE_ERROR && !type_is_valid_logical_operand(t)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-08",
                                   "Logical operator requires boolean operands");
                    return SEM_TYPE_ERROR;
                }
                return SEM_TYPE_BOOL;
            }

            if (op == TOKEN_PLUS || op == TOKEN_MINUS) {
                SemType t = semantic_check_expression(ctx, operand);
                if (t != SEM_TYPE_INT && t != SEM_TYPE_FLOAT) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-05",
                                   "Invalid operand types for arithmetic");
                    return SEM_TYPE_ERROR;
                }
                return t;
            }

            if (op == TOKEN_INCREMENT || op == TOKEN_DECREMENT) {
                if (operand->kind != AST_IDENTIFIER && operand->kind != AST_ARRAY_ACCESS) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-05",
                                   "Increment/decrement operand must be an lvalue");
                    return SEM_TYPE_ERROR;
                }
                SemType t = semantic_check_expression(ctx, operand);
                if (t != SEM_TYPE_ERROR && !type_is_valid_increment_type(t)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-05",
                                   "Increment/decrement requires modifiable int or float operand");
                    return SEM_TYPE_ERROR;
                }
                return t;
            }

            if (op == TOKEN_AMPERSAND) {
                semantic_error(ctx, expr->line, expr->column, "SEM-18",
                               "Address-of operator '&' is restricted to scanf destination notation only");
                if (operand) {
                    semantic_check_expression(ctx, operand);
                }
                return SEM_TYPE_ERROR;
            }

            return SEM_TYPE_ERROR;
        }

        case AST_ASSIGNMENT: {
            const ASTNode *left = expr->data.assignment.left;
            const ASTNode *right = expr->data.assignment.right;
            TokenType op = expr->data.assignment.op;

            if (left->kind != AST_IDENTIFIER && left->kind != AST_ARRAY_ACCESS) {
                semantic_error(ctx, expr->line, expr->column, "SEM-04",
                               "Left-hand side of assignment must be an lvalue");
                return SEM_TYPE_ERROR;
            }

            SemType ltype = semantic_check_expression(ctx, left);
            SemType rtype = semantic_check_expression(ctx, right);

            if (ltype == SEM_TYPE_ERROR || rtype == SEM_TYPE_ERROR) {
                return SEM_TYPE_ERROR;
            }

            if (op == TOKEN_ASSIGN) {
                if (!type_is_valid_assignment(ltype, rtype)) {
                    if (left->kind == AST_ARRAY_ACCESS) {
                        semantic_error(ctx, expr->line, expr->column, "SEM-16",
                                       "Element assignment incompatible with %s", sem_type_name(ltype));
                    } else {
                        semantic_error(ctx, expr->line, expr->column, "SEM-04",
                                       "Cannot assign %s to %s", sem_type_name(rtype), sem_type_name(ltype));
                    }
                    return SEM_TYPE_ERROR;
                }
            } else {
                if (!type_is_valid_compound_assignment(op, ltype, rtype)) {
                    if (left->kind == AST_ARRAY_ACCESS) {
                        semantic_error(ctx, expr->line, expr->column, "SEM-16",
                                       "Element assignment incompatible with %s", sem_type_name(ltype));
                    } else {
                        semantic_error(ctx, expr->line, expr->column, "SEM-04",
                                       "Compound assignment incompatible with %s", sem_type_name(ltype));
                    }
                    return SEM_TYPE_ERROR;
                }
            }
            return ltype;
        }

        case AST_CALL: {
            const char *callee = expr->data.call.callee;
            Symbol *func_sym = scope_lookup(ctx->current_scope, callee);
            if (!func_sym) {
                if (symbol_exists_in_any_scope(ctx->global_scope, callee)) {
                    semantic_error(ctx, expr->line, expr->column, "SEM-03",
                                   "Identifier '%s' is out of scope", callee);
                } else {
                    semantic_error(ctx, expr->line, expr->column, "SEM-01",
                                   "Identifier '%s' is not declared", callee);
                }
                return SEM_TYPE_ERROR;
            }

            if (func_sym->kind != SYMBOL_FUNCTION && func_sym->kind != SYMBOL_PREDEFINED_FUNCTION) {
                semantic_error(ctx, expr->line, expr->column, "SEM-01",
                               "Identifier '%s' is not a function", callee);
                return SEM_TYPE_ERROR;
            }

            /* Predefined function (printf, scanf) */
            if (func_sym->kind == SYMBOL_PREDEFINED_FUNCTION) {
                if (strcmp(callee, "printf") == 0) {
                    return validate_printf_call(ctx, expr);
                } else if (strcmp(callee, "scanf") == 0) {
                    return validate_scanf_call(ctx, expr);
                }
                for (int i = 0; i < expr->data.call.arg_count; i++) {
                    semantic_check_expression(ctx, expr->data.call.args[i]);
                }
                return sem_type_from_ast_type(func_sym->return_type);
            }

            /* User function: validate arity */
            if (expr->data.call.arg_count != func_sym->param_count) {
                semantic_error(ctx, expr->line, expr->column, "SEM-09",
                               "Wrong number of arguments (expected %d, got %d)",
                               func_sym->param_count, expr->data.call.arg_count);
                return sem_type_from_ast_type(func_sym->return_type);
            }

            /* Validate argument types */
            for (int i = 0; i < expr->data.call.arg_count; i++) {
                SemType arg_t = semantic_check_expression(ctx, expr->data.call.args[i]);
                SemType param_t = sem_type_from_ast_type(func_sym->params[i].type);
                if (arg_t != SEM_TYPE_ERROR && !type_can_implicitly_convert(arg_t, param_t)) {
                    semantic_error(ctx, expr->data.call.args[i]->line, expr->data.call.args[i]->column, "SEM-10",
                                   "Argument type mismatch for parameter '%s' (expected %s, got %s)",
                                   func_sym->params[i].name, sem_type_name(param_t), sem_type_name(arg_t));
                }
            }

            return sem_type_from_ast_type(func_sym->return_type);
        }

        case AST_CAST: {
            SemType target_t = sem_type_from_ast_type(expr->data.cast.target_type);
            SemType src_t = semantic_check_expression(ctx, expr->data.cast.operand);
            if (src_t != SEM_TYPE_ERROR && !type_can_explicit_cast(src_t, target_t)) {
                semantic_error(ctx, expr->line, expr->column, "SEM-19",
                               "Cast type combination is not supported");
                return SEM_TYPE_ERROR;
            }
            return target_t;
        }

        default:
            return SEM_TYPE_ERROR;
    }
}

/*
 * Statement Validation Visitor
 */
static void validate_statement(SemanticContext *ctx, const ASTNode *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case AST_DECLARATION: {
            const char *name = stmt->data.declaration.name;
            if (scope_lookup_current(ctx->current_scope, name) != NULL) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-02",
                               "Duplicate declaration of '%s'", name);
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

            if (stmt->data.declaration.initializer) {
                SemType decl_t = sem_type_from_ast_type(stmt->data.declaration.type);
                SemType init_t = semantic_check_expression(ctx, stmt->data.declaration.initializer);
                if (init_t != SEM_TYPE_ERROR && !type_is_valid_assignment(decl_t, init_t)) {
                    semantic_error(ctx, stmt->line, stmt->column, "SEM-04",
                                   "Cannot assign %s to %s", sem_type_name(init_t), sem_type_name(decl_t));
                }
            }
            break;
        }

        case AST_BLOCK: {
            ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_BLOCK, "block");
            for (int i = 0; i < stmt->data.block.stmt_count; i++) {
                validate_statement(ctx, stmt->data.block.statements[i]);
            }
            ctx->current_scope = scope_exit(ctx->current_scope);
            break;
        }

        case AST_IF: {
            SemType cond_t = semantic_check_expression(ctx, stmt->data.if_stmt.condition);
            if (cond_t != SEM_TYPE_ERROR && cond_t != SEM_TYPE_BOOL) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-07",
                               "Condition must produce boolean result");
            }
            validate_statement(ctx, stmt->data.if_stmt.then_branch);
            if (stmt->data.if_stmt.else_branch) {
                validate_statement(ctx, stmt->data.if_stmt.else_branch);
            }
            break;
        }

        case AST_WHILE: {
            ctx->loop_depth++;
            SemType cond_t = semantic_check_expression(ctx, stmt->data.while_stmt.condition);
            if (cond_t != SEM_TYPE_ERROR && cond_t != SEM_TYPE_BOOL) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-07",
                               "Condition must produce boolean result");
            }
            validate_statement(ctx, stmt->data.while_stmt.body);
            ctx->loop_depth--;
            break;
        }

        case AST_DO_WHILE: {
            ctx->loop_depth++;
            validate_statement(ctx, stmt->data.while_stmt.body);
            SemType cond_t = semantic_check_expression(ctx, stmt->data.while_stmt.condition);
            if (cond_t != SEM_TYPE_ERROR && cond_t != SEM_TYPE_BOOL) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-07",
                               "Condition must produce boolean result");
            }
            ctx->loop_depth--;
            break;
        }

        case AST_FOR: {
            ctx->loop_depth++;
            if (stmt->data.for_stmt.init && stmt->data.for_stmt.init->kind == AST_DECLARATION) {
                ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_BLOCK, "for");
                validate_statement(ctx, stmt->data.for_stmt.init);
            } else if (stmt->data.for_stmt.init) {
                semantic_check_expression(ctx, stmt->data.for_stmt.init);
            }

            if (stmt->data.for_stmt.condition) {
                SemType cond_t = semantic_check_expression(ctx, stmt->data.for_stmt.condition);
                if (cond_t != SEM_TYPE_ERROR && cond_t != SEM_TYPE_BOOL) {
                    semantic_error(ctx, stmt->line, stmt->column, "SEM-07",
                                   "Condition must produce boolean result");
                }
            }

            if (stmt->data.for_stmt.update) {
                semantic_check_expression(ctx, stmt->data.for_stmt.update);
            }

            validate_statement(ctx, stmt->data.for_stmt.body);

            if (stmt->data.for_stmt.init && stmt->data.for_stmt.init->kind == AST_DECLARATION) {
                ctx->current_scope = scope_exit(ctx->current_scope);
            }
            ctx->loop_depth--;
            break;
        }

        case AST_RETURN: {
            if (!ctx->current_function) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-11",
                               "Return statement outside function");
                return;
            }

            SemType expected_t = sem_type_from_ast_type(ctx->current_function->return_type);
            if (expected_t == SEM_TYPE_VOID) {
                if (stmt->data.return_stmt.expression != NULL) {
                    semantic_error(ctx, stmt->line, stmt->column, "SEM-11",
                                   "Return expression incompatible with function type");
                }
            } else {
                if (stmt->data.return_stmt.expression == NULL) {
                    semantic_error(ctx, stmt->line, stmt->column, "SEM-11",
                                   "Return expression incompatible with function type");
                } else {
                    SemType actual_t = semantic_check_expression(ctx, stmt->data.return_stmt.expression);
                    if (actual_t != SEM_TYPE_ERROR && !type_is_valid_return(expected_t, actual_t)) {
                        semantic_error(ctx, stmt->line, stmt->column, "SEM-11",
                                       "Return expression incompatible with function type");
                    } else if (actual_t != SEM_TYPE_ERROR) {
                        ctx->function_has_return = 1;
                    }
                }
            }
            break;
        }

        case AST_BREAK: {
            if (ctx->loop_depth <= 0) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-13",
                               "break is only valid inside a loop");
            }
            break;
        }

        case AST_CONTINUE: {
            if (ctx->loop_depth <= 0) {
                semantic_error(ctx, stmt->line, stmt->column, "SEM-14",
                               "continue is only valid inside a loop");
            }
            break;
        }

        default:
            /* Expression statements (call, assignment, etc.) */
            semantic_check_expression(ctx, stmt);
            break;
    }
}

/*
 * Scope building pass (Stage 7 backward-compatible helper)
 */
int semantic_build_symbols(SemanticContext *ctx, const ASTNode *program) {
    return semantic_analyze(ctx, program);
}

/*
 * Full Semantic Analysis entry point (Stage 9)
 */
int semantic_analyze(SemanticContext *ctx, const ASTNode *program) {
    if (!ctx || !program || program->kind != AST_PROGRAM) {
        return -1;
    }

    ctx->current_scope = ctx->global_scope;
    ctx->main_count = 0;

    for (int i = 0; i < program->data.program.decl_count; i++) {
        const ASTNode *decl = program->data.program.declarations[i];
        if (!decl) continue;

        if (decl->kind == AST_DECLARATION) {
            validate_statement(ctx, decl);
        } else if (decl->kind == AST_FUNCTION) {
            const char *func_name = decl->data.function.name;

            if (scope_lookup_current(ctx->global_scope, func_name) != NULL) {
                semantic_error(ctx, decl->line, decl->column, "SEM-02",
                               "Duplicate declaration of '%s'", func_name);
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

            /* Entry point check for main() */
            if (strcmp(func_name, "main") == 0) {
                ctx->main_count++;
                if (decl->data.function.return_type != TYPE_INT || decl->data.function.param_count != 0) {
                    semantic_error(ctx, decl->line, decl->column, "SEM-20",
                                   "Entry point must be int main()");
                }
            }

            /* Enter function scope */
            ctx->current_scope = scope_enter(ctx->current_scope, SCOPE_FUNCTION, func_name);
            ctx->current_function = scope_lookup_current(ctx->global_scope, func_name);
            ctx->function_has_return = 0;

            /* Insert parameters */
            for (int p = 0; p < decl->data.function.param_count; p++) {
                const char *pname = decl->data.function.params[p].name;
                ASTType ptype = decl->data.function.params[p].type;
                if (scope_lookup_current(ctx->current_scope, pname) != NULL) {
                    semantic_error(ctx, decl->line, decl->column, "SEM-02",
                                   "Duplicate declaration of '%s'", pname);
                } else {
                    Symbol *psym = symbol_create_parameter(pname, ptype, decl->line, decl->column);
                    scope_insert(ctx->current_scope, psym);
                }
            }

            /* Traverse function body statements */
            if (decl->data.function.body && decl->data.function.body->kind == AST_BLOCK) {
                const ASTNode *body = decl->data.function.body;
                for (int s = 0; s < body->data.block.stmt_count; s++) {
                    validate_statement(ctx, body->data.block.statements[s]);
                }
            }

            /* Non-void return completeness check */
            if (decl->data.function.return_type != TYPE_VOID && !ctx->function_has_return) {
                semantic_error(ctx, decl->line, decl->column, "SEM-12",
                               "Non-void function requires compatible return");
            }

            ctx->current_scope = scope_exit(ctx->current_scope);
            ctx->current_function = NULL;
        }
    }

    /* Verify main entry point exists */
    if (ctx->main_count == 0) {
        semantic_error(ctx, 1, 1, "SEM-20",
                       "Entry point must be int main() (missing main)");
    }

    return ctx->error_count;
}
