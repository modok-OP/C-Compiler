#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
 * Error reporting helper
 */
static void parser_error(Parser *parser, const char *fmt, ...) {
    parser->has_error = 1;
    parser->error_count++;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Syntax Error [%d:%d]: ", parser->current.line, parser->current.column);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/*
 * Parser lifecycle
 */
Parser *parser_create(Lexer *lexer) {
    if (!lexer) return NULL;

    Parser *parser = (Parser *)malloc(sizeof(Parser));
    if (!parser) {
        fprintf(stderr, "Fatal error: Out of memory allocating Parser\n");
        exit(1);
    }

    parser->lexer = lexer;
    parser->has_error = 0;
    parser->error_count = 0;
    parser->current = lexer_next_token(lexer);
    parser->next = lexer_next_token(lexer);

    return parser;
}

void parser_destroy(Parser *parser) {
    if (!parser) return;
    token_free(&parser->current);
    token_free(&parser->next);
    free(parser);
}

static Token parser_advance(Parser *parser) {
    Token prev = parser->current;
    parser->current = parser->next;
    parser->next = lexer_next_token(parser->lexer);
    return prev;
}

static int is_assignment_operator(TokenType type) {
    return (type == TOKEN_ASSIGN ||
            type == TOKEN_PLUS_ASSIGN ||
            type == TOKEN_MINUS_ASSIGN ||
            type == TOKEN_MULTIPLY_ASSIGN ||
            type == TOKEN_DIVIDE_ASSIGN ||
            type == TOKEN_MODULO_ASSIGN);
}

static int is_type_token(TokenType type) {
    return (type == TOKEN_INT ||
            type == TOKEN_FLOAT ||
            type == TOKEN_CHAR ||
            type == TOKEN_VOID);
}

static ASTType token_to_ast_type(TokenType type) {
    switch (type) {
        case TOKEN_INT:   return TYPE_INT;
        case TOKEN_FLOAT: return TYPE_FLOAT;
        case TOKEN_CHAR:  return TYPE_CHAR;
        case TOKEN_VOID:  return TYPE_VOID;
        default:          return TYPE_INT;
    }
}

static char unescape_char_literal(const char *lexeme) {
    if (!lexeme || strlen(lexeme) < 2) return '\0';
    if (lexeme[1] == '\\') {
        switch (lexeme[2]) {
            case 'n': return '\n';
            case 't': return '\t';
            case '\\': return '\\';
            case '\'': return '\'';
            case '\"': return '\"';
            default: return lexeme[2];
        }
    }
    return lexeme[1];
}

static char *unescape_string_literal(const char *lexeme) {
    if (!lexeme) return NULL;
    size_t len = strlen(lexeme);
    if (len < 2) {
        char *empty = (char *)malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    char *out = (char *)malloc(len);
    if (!out) {
        fprintf(stderr, "Fatal error: Out of memory unescaping string literal\n");
        exit(1);
    }

    size_t j = 0;
    for (size_t i = 1; i < len - 1; i++) {
        if (lexeme[i] == '\\' && i + 1 < len - 1) {
            i++;
            switch (lexeme[i]) {
                case 'n':  out[j++] = '\n'; break;
                case 't':  out[j++] = '\t'; break;
                case '\\': out[j++] = '\\'; break;
                case '\'': out[j++] = '\''; break;
                case '\"': out[j++] = '\"'; break;
                default:   out[j++] = lexeme[i]; break;
            }
        } else {
            out[j++] = lexeme[i];
        }
    }
    out[j] = '\0';
    return out;
}

/*
 * Entry point: expression -> assignment_expression
 */
ASTNode *parser_parse_expression(Parser *parser) {
    return parser_parse_assignment_expr(parser);
}

/*
 * assignment_expression -> logical_or_expression assignment_tail
 * assignment_tail -> assignment_operator assignment_expression | ε
 * (Right-associative)
 */
ASTNode *parser_parse_assignment_expr(Parser *parser) {
    ASTNode *left = parser_parse_logical_or_expr(parser);
    if (!left) return NULL;

    if (is_assignment_operator(parser->current.type)) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_assignment_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        ASTNode *assign = ast_create_assignment(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
        return assign;
    }

    return left;
}

/*
 * logical_or_expression -> logical_and_expression ('||' logical_and_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_logical_or_expr(Parser *parser) {
    ASTNode *left = parser_parse_logical_and_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_LOGICAL_OR) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_logical_and_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * logical_and_expression -> equality_expression ('&&' equality_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_logical_and_expr(Parser *parser) {
    ASTNode *left = parser_parse_equality_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_LOGICAL_AND) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_equality_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * equality_expression -> relational_expression (('==' | '!=') relational_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_equality_expr(Parser *parser) {
    ASTNode *left = parser_parse_relational_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_EQUAL || parser->current.type == TOKEN_NOT_EQUAL) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_relational_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * relational_expression -> additive_expression (('<' | '>' | '<=' | '>=') additive_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_relational_expr(Parser *parser) {
    ASTNode *left = parser_parse_additive_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_LESS ||
           parser->current.type == TOKEN_GREATER ||
           parser->current.type == TOKEN_LESS_EQUAL ||
           parser->current.type == TOKEN_GREATER_EQUAL) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_additive_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * additive_expression -> multiplicative_expression (('+' | '-') multiplicative_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_additive_expr(Parser *parser) {
    ASTNode *left = parser_parse_multiplicative_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_PLUS || parser->current.type == TOKEN_MINUS) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_multiplicative_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * multiplicative_expression -> unary_expression (('*' | '/' | '%') unary_expression)*
 * (Left-associative)
 */
ASTNode *parser_parse_multiplicative_expr(Parser *parser) {
    ASTNode *left = parser_parse_unary_expr(parser);
    if (!left) return NULL;

    while (parser->current.type == TOKEN_MULTIPLY ||
           parser->current.type == TOKEN_DIVIDE ||
           parser->current.type == TOKEN_MODULO) {
        Token op_tok = parser_advance(parser);
        ASTNode *right = parser_parse_unary_expr(parser);
        if (!right) {
            ast_free(left);
            token_free(&op_tok);
            return NULL;
        }
        left = ast_create_binary(op_tok.type, left, right, op_tok.line, op_tok.column);
        token_free(&op_tok);
    }

    return left;
}

/*
 * unary_expression
 *  -> '++' unary_expression
 *   | '--' unary_expression
 *   | ('+' | '-' | '!' | '&') unary_expression
 *   | cast_expression
 */
ASTNode *parser_parse_unary_expr(Parser *parser) {
    TokenType type = parser->current.type;

    if (type == TOKEN_INCREMENT ||
        type == TOKEN_DECREMENT ||
        type == TOKEN_PLUS ||
        type == TOKEN_MINUS ||
        type == TOKEN_LOGICAL_NOT ||
        type == TOKEN_AMPERSAND) {
        Token op_tok = parser_advance(parser);
        ASTNode *operand = parser_parse_unary_expr(parser);
        if (!operand) {
            token_free(&op_tok);
            return NULL;
        }
        ASTNode *unary = ast_create_unary(op_tok.type, 0 /* is_postfix = false */, operand, op_tok.line, op_tok.column);
        token_free(&op_tok);
        return unary;
    }

    return parser_parse_cast_expr(parser);
}

/*
 * cast_expression
 *  -> '(' type ')' cast_expression
 *   | postfix_expression
 */
ASTNode *parser_parse_cast_expr(Parser *parser) {
    /* Check for '(' type ')' */
    if (parser->current.type == TOKEN_LEFT_PAREN && is_type_token(parser->next.type)) {
        int line = parser->current.line;
        int col = parser->current.column;

        Token lp = parser_advance(parser); /* consume '(' */
        token_free(&lp);

        Token type_tok = parser_advance(parser); /* consume type */
        ASTType target_type = token_to_ast_type(type_tok.type);
        token_free(&type_tok);

        if (parser->current.type != TOKEN_RIGHT_PAREN) {
            parser_error(parser, "expected ')' after type in cast expression but found '%s'",
                         parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            return NULL;
        }

        Token rp = parser_advance(parser); /* consume ')' */
        token_free(&rp);

        ASTNode *operand = parser_parse_cast_expr(parser);
        if (!operand) return NULL;

        return ast_create_cast(target_type, operand, line, col);
    }

    return parser_parse_postfix_expr(parser);
}

/*
 * postfix_expression
 *  -> primary_expression postfix_tail*
 * postfix_tail
 *  -> '++'
 *   | '--'
 *   | '[' expression ']'
 *   | '(' argument_list_opt ')'
 */
ASTNode *parser_parse_postfix_expr(Parser *parser) {
    ASTNode *expr = parser_parse_primary_expr(parser);
    if (!expr) return NULL;

    while (1) {
        if (parser->current.type == TOKEN_INCREMENT) {
            Token op_tok = parser_advance(parser);
            expr = ast_create_unary(TOKEN_INCREMENT, 1 /* is_postfix = true */, expr, op_tok.line, op_tok.column);
            token_free(&op_tok);
        } else if (parser->current.type == TOKEN_DECREMENT) {
            Token op_tok = parser_advance(parser);
            expr = ast_create_unary(TOKEN_DECREMENT, 1 /* is_postfix = true */, expr, op_tok.line, op_tok.column);
            token_free(&op_tok);
        } else if (parser->current.type == TOKEN_LEFT_BRACKET) {
            Token lb = parser_advance(parser);
            token_free(&lb);

            ASTNode *index_expr = parser_parse_expression(parser);
            if (!index_expr) {
                ast_free(expr);
                return NULL;
            }

            if (parser->current.type != TOKEN_RIGHT_BRACKET) {
                parser_error(parser, "expected ']' after array index expression but found '%s'",
                             parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                ast_free(expr);
                ast_free(index_expr);
                return NULL;
            }

            Token rb = parser_advance(parser);
            token_free(&rb);

            expr = ast_create_array_access(expr, index_expr, expr->line, expr->column);
        } else if (parser->current.type == TOKEN_LEFT_PAREN) {
            Token lp = parser_advance(parser);
            token_free(&lp);

            if (expr->kind != AST_IDENTIFIER) {
                parser_error(parser, "called object is not a function identifier");
                ast_free(expr);
                return NULL;
            }

            ASTNode *call = ast_create_call(expr->data.identifier.name, expr->line, expr->column);
            ast_free(expr);

            if (parser->current.type == TOKEN_RIGHT_PAREN) {
                Token rp = parser_advance(parser);
                token_free(&rp);
            } else {
                while (1) {
                    ASTNode *arg = parser_parse_assignment_expr(parser);
                    if (!arg) {
                        ast_free(call);
                        return NULL;
                    }
                    ast_call_add_arg(call, arg);

                    if (parser->current.type == TOKEN_COMMA) {
                        Token comma = parser_advance(parser);
                        token_free(&comma);
                    } else {
                        break;
                    }
                }

                if (parser->current.type != TOKEN_RIGHT_PAREN) {
                    parser_error(parser, "expected ')' after call arguments but found '%s'",
                                 parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                    ast_free(call);
                    return NULL;
                }

                Token rp = parser_advance(parser);
                token_free(&rp);
            }

            expr = call;
        } else {
            break;
        }
    }

    return expr;
}

/*
 * primary_expression
 *  -> identifier
 *   | integer_literal
 *   | float_literal
 *   | char_literal
 *   | string_literal
 *   | '(' expression ')'
 */
ASTNode *parser_parse_primary_expr(Parser *parser) {
    Token tok = parser->current;

    switch (tok.type) {
        case TOKEN_IDENTIFIER: {
            Token id_tok = parser_advance(parser);
            ASTNode *node = ast_create_identifier(id_tok.lexeme, id_tok.line, id_tok.column);
            token_free(&id_tok);
            return node;
        }

        case TOKEN_INT_LITERAL: {
            Token lit_tok = parser_advance(parser);
            int val = atoi(lit_tok.lexeme);
            ASTNode *node = ast_create_literal_int(val, lit_tok.lexeme, lit_tok.line, lit_tok.column);
            token_free(&lit_tok);
            return node;
        }

        case TOKEN_FLOAT_LITERAL: {
            Token lit_tok = parser_advance(parser);
            double val = atof(lit_tok.lexeme);
            ASTNode *node = ast_create_literal_float(val, lit_tok.lexeme, lit_tok.line, lit_tok.column);
            token_free(&lit_tok);
            return node;
        }

        case TOKEN_CHAR_LITERAL: {
            Token lit_tok = parser_advance(parser);
            char val = unescape_char_literal(lit_tok.lexeme);
            ASTNode *node = ast_create_literal_char(val, lit_tok.lexeme, lit_tok.line, lit_tok.column);
            token_free(&lit_tok);
            return node;
        }

        case TOKEN_STRING_LITERAL: {
            Token lit_tok = parser_advance(parser);
            char *str_val = unescape_string_literal(lit_tok.lexeme);
            ASTNode *node = ast_create_literal_string(str_val, lit_tok.lexeme, lit_tok.line, lit_tok.column);
            free(str_val);
            token_free(&lit_tok);
            return node;
        }

        case TOKEN_LEFT_PAREN: {
            Token lp = parser_advance(parser);
            token_free(&lp);

            ASTNode *inner = parser_parse_expression(parser);
            if (!inner) return NULL;

            if (parser->current.type != TOKEN_RIGHT_PAREN) {
                parser_error(parser, "expected ')' to close parenthesized expression but found '%s'",
                             parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                ast_free(inner);
                return NULL;
            }

            Token rp = parser_advance(parser);
            token_free(&rp);

            /* Return child expression without adding unnecessary punctuation nodes */
            return inner;
        }

        default:
            parser_error(parser, "unexpected token '%s', expected expression",
                         tok.lexeme && strlen(tok.lexeme) > 0 ? tok.lexeme : token_type_name(tok.type));
            return NULL;
    }
}
