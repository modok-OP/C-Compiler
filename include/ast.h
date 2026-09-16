#ifndef AST_H
#define AST_H

#include "token.h"

/*
 * AST Node Kinds required by Section 24 of the frozen specification.
 */
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_DO_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CAST,
    AST_LITERAL,
    AST_IDENTIFIER
} ASTNodeKind;

/*
 * Data types supported by the frozen specification (Section 9).
 */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_VOID
} ASTType;

/*
 * Literal categories supported by the frozen specification (Section 8).
 */
typedef enum {
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_CHAR,
    LITERAL_STRING
} ASTLiteralType;

/*
 * Function parameter representation.
 */
typedef struct {
    ASTType type;
    char *name;
} ASTParam;

/*
 * Forward declaration of ASTNode.
 */
typedef struct ASTNode ASTNode;

/*
 * AST Node structure.
 * Contains node kind, source coordinates, and kind-specific payload.
 */
struct ASTNode {
    ASTNodeKind kind;
    int line;
    int column;

    union {
        /* AST_PROGRAM: list of external declarations */
        struct {
            ASTNode **declarations;
            int decl_count;
            int decl_capacity;
        } program;

        /* AST_FUNCTION: function definition */
        struct {
            char *name;
            ASTType return_type;
            ASTParam *params;
            int param_count;
            int param_capacity;
            ASTNode *body;              /* AST_BLOCK */
        } function;

        /* AST_BLOCK: ordered list of statements */
        struct {
            ASTNode **statements;
            int stmt_count;
            int stmt_capacity;
        } block;

        /* AST_DECLARATION: variable or 1-D array declaration */
        struct {
            ASTType type;
            char *name;
            ASTNode *initializer;       /* Optional, NULL if none */
            int is_array;               /* 1 if array, 0 if scalar */
            int array_size;             /* Array dimension */
        } declaration;

        /* AST_ASSIGNMENT: assignment expression */
        struct {
            TokenType op;               /* =, +=, -=, *=, /=, %= */
            ASTNode *left;
            ASTNode *right;
        } assignment;

        /* AST_IF: if or if-else conditional */
        struct {
            ASTNode *condition;
            ASTNode *then_branch;
            ASTNode *else_branch;       /* Optional, NULL if none */
        } if_stmt;

        /* AST_WHILE, AST_DO_WHILE: while / do-while loops */
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_stmt;

        /* AST_FOR: for loop */
        struct {
            ASTNode *init;              /* Optional init declaration/expression */
            ASTNode *condition;         /* Optional loop condition */
            ASTNode *update;            /* Optional loop update expression */
            ASTNode *body;
        } for_stmt;

        /* AST_RETURN: return statement */
        struct {
            ASTNode *expression;        /* Optional, NULL for return; */
        } return_stmt;

        /* AST_CALL: function call */
        struct {
            char *callee;
            ASTNode **args;
            int arg_count;
            int arg_capacity;
        } call;

        /* AST_ARRAY_ACCESS: 1-D array subscripting */
        struct {
            ASTNode *array_expr;
            ASTNode *index_expr;
        } array_access;

        /* AST_BINARY_EXPR: binary operator expression */
        struct {
            TokenType op;
            ASTNode *left;
            ASTNode *right;
        } binary_expr;

        /* AST_UNARY_EXPR: unary operator expression */
        struct {
            TokenType op;
            int is_postfix;             /* 1 for postfix ++/--, 0 for prefix */
            ASTNode *operand;
        } unary_expr;

        /* AST_CAST: explicit type cast */
        struct {
            ASTType target_type;
            ASTNode *operand;
        } cast;

        /* AST_LITERAL: literal value */
        struct {
            ASTLiteralType literal_type;
            union {
                int int_val;
                double float_val;
                char char_val;
                char *string_val;
            } val;
            char *raw_lexeme;
        } literal;

        /* AST_IDENTIFIER: identifier name */
        struct {
            char *name;
        } identifier;
    } data;
};

/*
 * Helper functions
 */

/* Returns readable string representation of an ASTType */
const char *ast_type_name(ASTType type);

/* Returns readable string representation of an ASTNodeKind */
const char *ast_node_kind_name(ASTNodeKind kind);

/*
 * AST Node Constructors
 */

ASTNode *ast_create_program(int line, int column);
void ast_program_add_decl(ASTNode *prog, ASTNode *decl);

ASTNode *ast_create_function(const char *name, ASTType return_type, int line, int column);
void ast_function_add_param(ASTNode *func, ASTType type, const char *name);
void ast_function_set_body(ASTNode *func, ASTNode *body);

ASTNode *ast_create_block(int line, int column);
void ast_block_add_stmt(ASTNode *block, ASTNode *stmt);

ASTNode *ast_create_declaration(ASTType type, const char *name, ASTNode *initializer, int is_array, int array_size, int line, int column);

ASTNode *ast_create_assignment(TokenType op, ASTNode *left, ASTNode *right, int line, int column);

ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch, int line, int column);

ASTNode *ast_create_while(ASTNode *condition, ASTNode *body, int line, int column);
ASTNode *ast_create_do_while(ASTNode *condition, ASTNode *body, int line, int column);

ASTNode *ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body, int line, int column);

ASTNode *ast_create_return(ASTNode *expression, int line, int column);
ASTNode *ast_create_break(int line, int column);
ASTNode *ast_create_continue(int line, int column);

ASTNode *ast_create_call(const char *callee, int line, int column);
void ast_call_add_arg(ASTNode *call, ASTNode *arg);

ASTNode *ast_create_array_access(ASTNode *array_expr, ASTNode *index_expr, int line, int column);

ASTNode *ast_create_binary(TokenType op, ASTNode *left, ASTNode *right, int line, int column);

ASTNode *ast_create_unary(TokenType op, int is_postfix, ASTNode *operand, int line, int column);

ASTNode *ast_create_cast(ASTType target_type, ASTNode *operand, int line, int column);

ASTNode *ast_create_literal_int(int val, const char *raw_lexeme, int line, int column);
ASTNode *ast_create_literal_float(double val, const char *raw_lexeme, int line, int column);
ASTNode *ast_create_literal_char(char val, const char *raw_lexeme, int line, int column);
ASTNode *ast_create_literal_string(const char *val, const char *raw_lexeme, int line, int column);

ASTNode *ast_create_identifier(const char *name, int line, int column);

/*
 * AST Debugging and Visualization
 */
void ast_print(const ASTNode *node, int indent);

/*
 * Recursive AST Memory Deallocation
 */
void ast_free(ASTNode *node);

#endif /* AST_H */
