#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ast_type_name(ASTType type) {
    switch (type) {
        case TYPE_INT:   return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR:  return "char";
        case TYPE_VOID:  return "void";
        default:         return "unknown_type";
    }
}

const char *ast_node_kind_name(ASTNodeKind kind) {
    switch (kind) {
        case AST_PROGRAM:      return "Program";
        case AST_FUNCTION:     return "Function";
        case AST_BLOCK:        return "Block";
        case AST_DECLARATION:  return "Declaration";
        case AST_ASSIGNMENT:   return "Assignment";
        case AST_IF:           return "If";
        case AST_WHILE:        return "While";
        case AST_DO_WHILE:     return "DoWhile";
        case AST_FOR:          return "For";
        case AST_RETURN:       return "Return";
        case AST_BREAK:        return "Break";
        case AST_CONTINUE:     return "Continue";
        case AST_CALL:         return "Call";
        case AST_ARRAY_ACCESS: return "ArrayAccess";
        case AST_BINARY_EXPR:  return "BinaryExpr";
        case AST_UNARY_EXPR:   return "UnaryExpr";
        case AST_CAST:         return "Cast";
        case AST_LITERAL:      return "Literal";
        case AST_IDENTIFIER:   return "Identifier";
        default:               return "UnknownNode";
    }
}

static char *copy_string(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Fatal error: Out of memory in copy_string.\n");
        exit(1);
    }
    memcpy(copy, str, len + 1);
    return copy;
}

static ASTNode *ast_node_alloc(ASTNodeKind kind, int line, int column) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Fatal error: Out of memory while allocating AST node.\n");
        exit(1);
    }
    memset(node, 0, sizeof(ASTNode));
    node->kind = kind;
    node->line = line;
    node->column = column;
    return node;
}

/* Program */
ASTNode *ast_create_program(int line, int column) {
    ASTNode *node = ast_node_alloc(AST_PROGRAM, line, column);
    node->data.program.decl_capacity = 8;
    node->data.program.decl_count = 0;
    node->data.program.declarations = (ASTNode **)malloc(sizeof(ASTNode *) * node->data.program.decl_capacity);
    if (!node->data.program.declarations) {
        fprintf(stderr, "Fatal error: Out of memory while allocating program declarations.\n");
        exit(1);
    }
    return node;
}

void ast_program_add_decl(ASTNode *prog, ASTNode *decl) {
    if (!prog || prog->kind != AST_PROGRAM || !decl) return;
    if (prog->data.program.decl_count >= prog->data.program.decl_capacity) {
        prog->data.program.decl_capacity *= 2;
        prog->data.program.declarations = (ASTNode **)realloc(prog->data.program.declarations,
            sizeof(ASTNode *) * prog->data.program.decl_capacity);
        if (!prog->data.program.declarations) {
            fprintf(stderr, "Fatal error: Out of memory reallocating program declarations.\n");
            exit(1);
        }
    }
    prog->data.program.declarations[prog->data.program.decl_count++] = decl;
}

/* Function */
ASTNode *ast_create_function(const char *name, ASTType return_type, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_FUNCTION, line, column);
    node->data.function.name = copy_string(name);
    node->data.function.return_type = return_type;
    node->data.function.param_capacity = 4;
    node->data.function.param_count = 0;
    node->data.function.params = (ASTParam *)malloc(sizeof(ASTParam) * node->data.function.param_capacity);
    if (!node->data.function.params) {
        fprintf(stderr, "Fatal error: Out of memory allocating function parameters.\n");
        exit(1);
    }
    node->data.function.body = NULL;
    return node;
}

void ast_function_add_param(ASTNode *func, ASTType type, const char *name) {
    if (!func || func->kind != AST_FUNCTION || !name) return;
    if (func->data.function.param_count >= func->data.function.param_capacity) {
        func->data.function.param_capacity *= 2;
        func->data.function.params = (ASTParam *)realloc(func->data.function.params,
            sizeof(ASTParam) * func->data.function.param_capacity);
        if (!func->data.function.params) {
            fprintf(stderr, "Fatal error: Out of memory reallocating function parameters.\n");
            exit(1);
        }
    }
    func->data.function.params[func->data.function.param_count].type = type;
    func->data.function.params[func->data.function.param_count].name = copy_string(name);
    func->data.function.param_count++;
}

void ast_function_set_body(ASTNode *func, ASTNode *body) {
    if (!func || func->kind != AST_FUNCTION) return;
    func->data.function.body = body;
}

/* Block */
ASTNode *ast_create_block(int line, int column) {
    ASTNode *node = ast_node_alloc(AST_BLOCK, line, column);
    node->data.block.stmt_capacity = 8;
    node->data.block.stmt_count = 0;
    node->data.block.statements = (ASTNode **)malloc(sizeof(ASTNode *) * node->data.block.stmt_capacity);
    if (!node->data.block.statements) {
        fprintf(stderr, "Fatal error: Out of memory allocating block statements.\n");
        exit(1);
    }
    return node;
}

void ast_block_add_stmt(ASTNode *block, ASTNode *stmt) {
    if (!block || block->kind != AST_BLOCK || !stmt) return;
    if (block->data.block.stmt_count >= block->data.block.stmt_capacity) {
        block->data.block.stmt_capacity *= 2;
        block->data.block.statements = (ASTNode **)realloc(block->data.block.statements,
            sizeof(ASTNode *) * block->data.block.stmt_capacity);
        if (!block->data.block.statements) {
            fprintf(stderr, "Fatal error: Out of memory reallocating block statements.\n");
            exit(1);
        }
    }
    block->data.block.statements[block->data.block.stmt_count++] = stmt;
}

/* Declaration */
ASTNode *ast_create_declaration(ASTType type, const char *name, ASTNode *initializer, int is_array, int array_size, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_DECLARATION, line, column);
    node->data.declaration.type = type;
    node->data.declaration.name = copy_string(name);
    node->data.declaration.initializer = initializer;
    node->data.declaration.is_array = is_array;
    node->data.declaration.array_size = array_size;
    return node;
}

/* Assignment */
ASTNode *ast_create_assignment(TokenType op, ASTNode *left, ASTNode *right, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_ASSIGNMENT, line, column);
    node->data.assignment.op = op;
    node->data.assignment.left = left;
    node->data.assignment.right = right;
    return node;
}

/* If */
ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_IF, line, column);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    return node;
}

/* While */
ASTNode *ast_create_while(ASTNode *condition, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_WHILE, line, column);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

/* DoWhile */
ASTNode *ast_create_do_while(ASTNode *condition, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_DO_WHILE, line, column);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

/* For */
ASTNode *ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_FOR, line, column);
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.update = update;
    node->data.for_stmt.body = body;
    return node;
}

/* Return */
ASTNode *ast_create_return(ASTNode *expression, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_RETURN, line, column);
    node->data.return_stmt.expression = expression;
    return node;
}

/* Break */
ASTNode *ast_create_break(int line, int column) {
    return ast_node_alloc(AST_BREAK, line, column);
}

/* Continue */
ASTNode *ast_create_continue(int line, int column) {
    return ast_node_alloc(AST_CONTINUE, line, column);
}

/* Call */
ASTNode *ast_create_call(const char *callee, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_CALL, line, column);
    node->data.call.callee = copy_string(callee);
    node->data.call.arg_capacity = 4;
    node->data.call.arg_count = 0;
    node->data.call.args = (ASTNode **)malloc(sizeof(ASTNode *) * node->data.call.arg_capacity);
    if (!node->data.call.args) {
        fprintf(stderr, "Fatal error: Out of memory allocating call arguments.\n");
        exit(1);
    }
    return node;
}

void ast_call_add_arg(ASTNode *call, ASTNode *arg) {
    if (!call || call->kind != AST_CALL || !arg) return;
    if (call->data.call.arg_count >= call->data.call.arg_capacity) {
        call->data.call.arg_capacity *= 2;
        call->data.call.args = (ASTNode **)realloc(call->data.call.args,
            sizeof(ASTNode *) * call->data.call.arg_capacity);
        if (!call->data.call.args) {
            fprintf(stderr, "Fatal error: Out of memory reallocating call arguments.\n");
            exit(1);
        }
    }
    call->data.call.args[call->data.call.arg_count++] = arg;
}

/* Array Access */
ASTNode *ast_create_array_access(ASTNode *array_expr, ASTNode *index_expr, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_ARRAY_ACCESS, line, column);
    node->data.array_access.array_expr = array_expr;
    node->data.array_access.index_expr = index_expr;
    return node;
}

/* Binary Expression */
ASTNode *ast_create_binary(TokenType op, ASTNode *left, ASTNode *right, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_BINARY_EXPR, line, column);
    node->data.binary_expr.op = op;
    node->data.binary_expr.left = left;
    node->data.binary_expr.right = right;
    return node;
}

/* Unary Expression */
ASTNode *ast_create_unary(TokenType op, int is_postfix, ASTNode *operand, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_UNARY_EXPR, line, column);
    node->data.unary_expr.op = op;
    node->data.unary_expr.is_postfix = is_postfix;
    node->data.unary_expr.operand = operand;
    return node;
}

/* Cast */
ASTNode *ast_create_cast(ASTType target_type, ASTNode *operand, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_CAST, line, column);
    node->data.cast.target_type = target_type;
    node->data.cast.operand = operand;
    return node;
}

/* Literals */
ASTNode *ast_create_literal_int(int val, const char *raw_lexeme, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_LITERAL, line, column);
    node->data.literal.literal_type = LITERAL_INT;
    node->data.literal.val.int_val = val;
    node->data.literal.raw_lexeme = copy_string(raw_lexeme);
    return node;
}

ASTNode *ast_create_literal_float(double val, const char *raw_lexeme, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_LITERAL, line, column);
    node->data.literal.literal_type = LITERAL_FLOAT;
    node->data.literal.val.float_val = val;
    node->data.literal.raw_lexeme = copy_string(raw_lexeme);
    return node;
}

ASTNode *ast_create_literal_char(char val, const char *raw_lexeme, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_LITERAL, line, column);
    node->data.literal.literal_type = LITERAL_CHAR;
    node->data.literal.val.char_val = val;
    node->data.literal.raw_lexeme = copy_string(raw_lexeme);
    return node;
}

ASTNode *ast_create_literal_string(const char *val, const char *raw_lexeme, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_LITERAL, line, column);
    node->data.literal.literal_type = LITERAL_STRING;
    node->data.literal.val.string_val = copy_string(val);
    node->data.literal.raw_lexeme = copy_string(raw_lexeme);
    return node;
}

/* Identifier */
ASTNode *ast_create_identifier(const char *name, int line, int column) {
    ASTNode *node = ast_node_alloc(AST_IDENTIFIER, line, column);
    node->data.identifier.name = copy_string(name);
    return node;
}

/*
 * Recursive AST Printing
 */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(const ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    print_indent(indent);

    switch (node->kind) {
        case AST_PROGRAM:
            printf("Program (line: %d, col: %d, count: %d)\n",
                   node->line, node->column, node->data.program.decl_count);
            for (int i = 0; i < node->data.program.decl_count; i++) {
                ast_print(node->data.program.declarations[i], indent + 1);
            }
            break;

        case AST_FUNCTION:
            printf("Function: %s -> %s (params: %d, line: %d, col: %d)\n",
                   node->data.function.name,
                   ast_type_name(node->data.function.return_type),
                   node->data.function.param_count,
                   node->line, node->column);
            for (int i = 0; i < node->data.function.param_count; i++) {
                print_indent(indent + 1);
                printf("Param: %s %s\n",
                       ast_type_name(node->data.function.params[i].type),
                       node->data.function.params[i].name);
            }
            if (node->data.function.body) {
                print_indent(indent + 1);
                printf("Body:\n");
                ast_print(node->data.function.body, indent + 2);
            }
            break;

        case AST_BLOCK:
            printf("Block (statements: %d, line: %d, col: %d)\n",
                   node->data.block.stmt_count, node->line, node->column);
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                ast_print(node->data.block.statements[i], indent + 1);
            }
            break;

        case AST_DECLARATION:
            if (node->data.declaration.is_array) {
                printf("Declaration: %s %s[%d] (line: %d, col: %d)\n",
                       ast_type_name(node->data.declaration.type),
                       node->data.declaration.name,
                       node->data.declaration.array_size,
                       node->line, node->column);
            } else {
                printf("Declaration: %s %s (line: %d, col: %d)\n",
                       ast_type_name(node->data.declaration.type),
                       node->data.declaration.name,
                       node->line, node->column);
            }
            if (node->data.declaration.initializer) {
                print_indent(indent + 1);
                printf("Initializer:\n");
                ast_print(node->data.declaration.initializer, indent + 2);
            }
            break;

        case AST_ASSIGNMENT:
            printf("Assignment: %s (line: %d, col: %d)\n",
                   token_type_name(node->data.assignment.op),
                   node->line, node->column);
            print_indent(indent + 1);
            printf("Left:\n");
            ast_print(node->data.assignment.left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            ast_print(node->data.assignment.right, indent + 2);
            break;

        case AST_IF:
            printf("If (line: %d, col: %d)\n", node->line, node->column);
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent + 1);
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_branch, indent + 2);
            }
            break;

        case AST_WHILE:
            printf("While (line: %d, col: %d)\n", node->line, node->column);
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;

        case AST_DO_WHILE:
            printf("DoWhile (line: %d, col: %d)\n", node->line, node->column);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            break;

        case AST_FOR:
            printf("For (line: %d, col: %d)\n", node->line, node->column);
            if (node->data.for_stmt.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                ast_print(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.condition) {
                print_indent(indent + 1);
                printf("Condition:\n");
                ast_print(node->data.for_stmt.condition, indent + 2);
            }
            if (node->data.for_stmt.update) {
                print_indent(indent + 1);
                printf("Update:\n");
                ast_print(node->data.for_stmt.update, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.for_stmt.body, indent + 2);
            break;

        case AST_RETURN:
            printf("Return (line: %d, col: %d)\n", node->line, node->column);
            if (node->data.return_stmt.expression) {
                ast_print(node->data.return_stmt.expression, indent + 1);
            }
            break;

        case AST_BREAK:
            printf("Break (line: %d, col: %d)\n", node->line, node->column);
            break;

        case AST_CONTINUE:
            printf("Continue (line: %d, col: %d)\n", node->line, node->column);
            break;

        case AST_CALL:
            printf("Call: %s (args: %d, line: %d, col: %d)\n",
                   node->data.call.callee,
                   node->data.call.arg_count,
                   node->line, node->column);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                ast_print(node->data.call.args[i], indent + 1);
            }
            break;

        case AST_ARRAY_ACCESS:
            printf("ArrayAccess (line: %d, col: %d)\n", node->line, node->column);
            print_indent(indent + 1);
            printf("Array:\n");
            ast_print(node->data.array_access.array_expr, indent + 2);
            print_indent(indent + 1);
            printf("Index:\n");
            ast_print(node->data.array_access.index_expr, indent + 2);
            break;

        case AST_BINARY_EXPR:
            printf("BinaryExpr: %s (line: %d, col: %d)\n",
                   token_type_name(node->data.binary_expr.op),
                   node->line, node->column);
            print_indent(indent + 1);
            printf("Left:\n");
            ast_print(node->data.binary_expr.left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            ast_print(node->data.binary_expr.right, indent + 2);
            break;

        case AST_UNARY_EXPR:
            printf("UnaryExpr: %s (postfix: %d, line: %d, col: %d)\n",
                   token_type_name(node->data.unary_expr.op),
                   node->data.unary_expr.is_postfix,
                   node->line, node->column);
            print_indent(indent + 1);
            printf("Operand:\n");
            ast_print(node->data.unary_expr.operand, indent + 2);
            break;

        case AST_CAST:
            printf("Cast: (%s) (line: %d, col: %d)\n",
                   ast_type_name(node->data.cast.target_type),
                   node->line, node->column);
            print_indent(indent + 1);
            printf("Operand:\n");
            ast_print(node->data.cast.operand, indent + 2);
            break;

        case AST_LITERAL:
            switch (node->data.literal.literal_type) {
                case LITERAL_INT:
                    printf("Literal: %d (int, raw: \"%s\", line: %d, col: %d)\n",
                           node->data.literal.val.int_val,
                           node->data.literal.raw_lexeme ? node->data.literal.raw_lexeme : "",
                           node->line, node->column);
                    break;
                case LITERAL_FLOAT:
                    printf("Literal: %g (float, raw: \"%s\", line: %d, col: %d)\n",
                           node->data.literal.val.float_val,
                           node->data.literal.raw_lexeme ? node->data.literal.raw_lexeme : "",
                           node->line, node->column);
                    break;
                case LITERAL_CHAR:
                    printf("Literal: '%c' (char, raw: \"%s\", line: %d, col: %d)\n",
                           node->data.literal.val.char_val,
                           node->data.literal.raw_lexeme ? node->data.literal.raw_lexeme : "",
                           node->line, node->column);
                    break;
                case LITERAL_STRING:
                    printf("Literal: \"%s\" (string, raw: \"%s\", line: %d, col: %d)\n",
                           node->data.literal.val.string_val ? node->data.literal.val.string_val : "",
                           node->data.literal.raw_lexeme ? node->data.literal.raw_lexeme : "",
                           node->line, node->column);
                    break;
            }
            break;

        case AST_IDENTIFIER:
            printf("Identifier: %s (line: %d, col: %d)\n",
                   node->data.identifier.name,
                   node->line, node->column);
            break;
    }
}

/*
 * Recursive AST Memory Deallocation
 */
void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            if (node->data.program.declarations) {
                for (int i = 0; i < node->data.program.decl_count; i++) {
                    ast_free(node->data.program.declarations[i]);
                }
                free(node->data.program.declarations);
            }
            break;

        case AST_FUNCTION:
            if (node->data.function.name) {
                free(node->data.function.name);
            }
            if (node->data.function.params) {
                for (int i = 0; i < node->data.function.param_count; i++) {
                    if (node->data.function.params[i].name) {
                        free(node->data.function.params[i].name);
                    }
                }
                free(node->data.function.params);
            }
            ast_free(node->data.function.body);
            break;

        case AST_BLOCK:
            if (node->data.block.statements) {
                for (int i = 0; i < node->data.block.stmt_count; i++) {
                    ast_free(node->data.block.statements[i]);
                }
                free(node->data.block.statements);
            }
            break;

        case AST_DECLARATION:
            if (node->data.declaration.name) {
                free(node->data.declaration.name);
            }
            ast_free(node->data.declaration.initializer);
            break;

        case AST_ASSIGNMENT:
            ast_free(node->data.assignment.left);
            ast_free(node->data.assignment.right);
            break;

        case AST_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_branch);
            ast_free(node->data.if_stmt.else_branch);
            break;

        case AST_WHILE:
        case AST_DO_WHILE:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;

        case AST_FOR:
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.condition);
            ast_free(node->data.for_stmt.update);
            ast_free(node->data.for_stmt.body);
            break;

        case AST_RETURN:
            ast_free(node->data.return_stmt.expression);
            break;

        case AST_BREAK:
        case AST_CONTINUE:
            /* No dynamic payload */
            break;

        case AST_CALL:
            if (node->data.call.callee) {
                free(node->data.call.callee);
            }
            if (node->data.call.args) {
                for (int i = 0; i < node->data.call.arg_count; i++) {
                    ast_free(node->data.call.args[i]);
                }
                free(node->data.call.args);
            }
            break;

        case AST_ARRAY_ACCESS:
            ast_free(node->data.array_access.array_expr);
            ast_free(node->data.array_access.index_expr);
            break;

        case AST_BINARY_EXPR:
            ast_free(node->data.binary_expr.left);
            ast_free(node->data.binary_expr.right);
            break;

        case AST_UNARY_EXPR:
            ast_free(node->data.unary_expr.operand);
            break;

        case AST_CAST:
            ast_free(node->data.cast.operand);
            break;

        case AST_LITERAL:
            if (node->data.literal.literal_type == LITERAL_STRING && node->data.literal.val.string_val) {
                free(node->data.literal.val.string_val);
            }
            if (node->data.literal.raw_lexeme) {
                free(node->data.literal.raw_lexeme);
            }
            break;

        case AST_IDENTIFIER:
            if (node->data.identifier.name) {
                free(node->data.identifier.name);
            }
            break;
    }

    free(node);
}
