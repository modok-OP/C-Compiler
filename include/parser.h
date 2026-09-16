#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/*
 * Parser structure holding lexer instance, 3-token lookahead (current, next, next2),
 * and error tracking state.
 */
typedef struct {
    Lexer *lexer;
    Token current;
    Token next;
    Token next2;
    int has_error;
    int error_count;
} Parser;

/*
 * Lifecycle functions
 */
Parser *parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);

/*
 * Top-level program and function parsing entry points (Stage 6)
 */
ASTNode *parser_parse_program(Parser *parser);
ASTNode *parser_parse_function_definition(Parser *parser);

/*
 * Expression parsing entry points (Stage 4)
 */
ASTNode *parser_parse_expression(Parser *parser);
ASTNode *parser_parse_assignment_expr(Parser *parser);
ASTNode *parser_parse_logical_or_expr(Parser *parser);
ASTNode *parser_parse_logical_and_expr(Parser *parser);
ASTNode *parser_parse_equality_expr(Parser *parser);
ASTNode *parser_parse_relational_expr(Parser *parser);
ASTNode *parser_parse_additive_expr(Parser *parser);
ASTNode *parser_parse_multiplicative_expr(Parser *parser);
ASTNode *parser_parse_unary_expr(Parser *parser);
ASTNode *parser_parse_cast_expr(Parser *parser);
ASTNode *parser_parse_postfix_expr(Parser *parser);
ASTNode *parser_parse_primary_expr(Parser *parser);

/*
 * Declarations and Statements parsing entry points (Stage 5)
 */
int parser_parse_declaration_list(Parser *parser, ASTNode ***out_decls, int *out_count);
ASTNode *parser_parse_declaration_no_semicolon(Parser *parser);
ASTNode *parser_parse_statement(Parser *parser);
ASTNode *parser_parse_block(Parser *parser);
ASTNode *parser_parse_if_stmt(Parser *parser);
ASTNode *parser_parse_while_stmt(Parser *parser);
ASTNode *parser_parse_do_while_stmt(Parser *parser);
ASTNode *parser_parse_for_stmt(Parser *parser);
ASTNode *parser_parse_return_stmt(Parser *parser);
ASTNode *parser_parse_break_stmt(Parser *parser);
ASTNode *parser_parse_continue_stmt(Parser *parser);

#endif /* PARSER_H */
