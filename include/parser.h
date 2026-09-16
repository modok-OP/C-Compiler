#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/*
 * Parser structure holding lexer instance, 2-token lookahead,
 * and error tracking state.
 */
typedef struct {
    Lexer *lexer;
    Token current;
    Token next;
    int has_error;
    int error_count;
} Parser;

/*
 * Lifecycle functions
 */
Parser *parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);

/*
 * Expression parsing entry point (top-level expression -> assignment_expression)
 */
ASTNode *parser_parse_expression(Parser *parser);

/*
 * Recursive descent parsing functions matching the frozen grammar hierarchy
 */
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

#endif /* PARSER_H */
