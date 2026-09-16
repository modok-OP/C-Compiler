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
 * Parser lifecycle with 3-token lookahead
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
    parser->next2 = lexer_next_token(lexer);

    return parser;
}

void parser_destroy(Parser *parser) {
    if (!parser) return;
    token_free(&parser->current);
    token_free(&parser->next);
    token_free(&parser->next2);
    free(parser);
}

static Token parser_advance(Parser *parser) {
    Token prev = parser->current;
    parser->current = parser->next;
    parser->next = parser->next2;
    parser->next2 = lexer_next_token(parser->lexer);
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
 * =========================================================================
 * Stage 4: Expression Parsing
 * =========================================================================
 */
ASTNode *parser_parse_expression(Parser *parser) {
    return parser_parse_assignment_expr(parser);
}

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

ASTNode *parser_parse_cast_expr(Parser *parser) {
    if (parser->current.type == TOKEN_LEFT_PAREN && is_type_token(parser->next.type)) {
        int line = parser->current.line;
        int col = parser->current.column;

        Token lp = parser_advance(parser);
        token_free(&lp);

        Token type_tok = parser_advance(parser);
        ASTType target_type = token_to_ast_type(type_tok.type);
        token_free(&type_tok);

        if (parser->current.type != TOKEN_RIGHT_PAREN) {
            parser_error(parser, "expected ')' after type in cast expression but found '%s'",
                         parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            return NULL;
        }

        Token rp = parser_advance(parser);
        token_free(&rp);

        ASTNode *operand = parser_parse_cast_expr(parser);
        if (!operand) return NULL;

        return ast_create_cast(target_type, operand, line, col);
    }

    return parser_parse_postfix_expr(parser);
}

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

            return inner;
        }

        default:
            parser_error(parser, "unexpected token '%s', expected expression",
                         tok.lexeme && strlen(tok.lexeme) > 0 ? tok.lexeme : token_type_name(tok.type));
            return NULL;
    }
}

/*
 * =========================================================================
 * Stage 5: Declarations and Statements
 * =========================================================================
 */

static ASTNode *parse_single_init_declarator(Parser *parser, ASTType type) {
    if (parser->current.type != TOKEN_IDENTIFIER) {
        parser_error(parser, "expected identifier in declaration but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }

    Token id_tok = parser_advance(parser);
    int line = id_tok.line;
    int col = id_tok.column;
    const char *name = id_tok.lexeme;

    if (parser->current.type == TOKEN_ASSIGN) {
        Token eq_tok = parser_advance(parser);
        token_free(&eq_tok);

        ASTNode *init_expr = parser_parse_assignment_expr(parser);
        if (!init_expr) {
            token_free(&id_tok);
            return NULL;
        }
        ASTNode *decl = ast_create_declaration(type, name, init_expr, 0, 0, line, col);
        token_free(&id_tok);
        return decl;
    } else if (parser->current.type == TOKEN_LEFT_BRACKET) {
        Token lb_tok = parser_advance(parser);
        token_free(&lb_tok);

        if (parser->current.type != TOKEN_INT_LITERAL) {
            parser_error(parser, "expected integer literal for array size but found '%s'",
                         parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            token_free(&id_tok);
            return NULL;
        }

        Token size_tok = parser_advance(parser);
        int array_size = atoi(size_tok.lexeme);
        token_free(&size_tok);

        if (parser->current.type != TOKEN_RIGHT_BRACKET) {
            parser_error(parser, "expected ']' after array size but found '%s'",
                         parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            token_free(&id_tok);
            return NULL;
        }

        Token rb_tok = parser_advance(parser);
        token_free(&rb_tok);

        ASTNode *decl = ast_create_declaration(type, name, NULL, 1, array_size, line, col);
        token_free(&id_tok);
        return decl;
    } else {
        ASTNode *decl = ast_create_declaration(type, name, NULL, 0, 0, line, col);
        token_free(&id_tok);
        return decl;
    }
}

ASTNode *parser_parse_declaration_no_semicolon(Parser *parser) {
    if (!is_type_token(parser->current.type)) {
        parser_error(parser, "expected type name in declaration");
        return NULL;
    }

    Token type_tok = parser_advance(parser);
    ASTType type = token_to_ast_type(type_tok.type);
    token_free(&type_tok);

    ASTNode *first_decl = parse_single_init_declarator(parser, type);
    if (!first_decl) return NULL;

    if (parser->current.type == TOKEN_COMMA) {
        ASTNode *block = ast_create_block(first_decl->line, first_decl->column);
        ast_block_add_stmt(block, first_decl);

        while (parser->current.type == TOKEN_COMMA) {
            Token comma = parser_advance(parser);
            token_free(&comma);

            ASTNode *next_decl = parse_single_init_declarator(parser, type);
            if (!next_decl) {
                ast_free(block);
                return NULL;
            }
            ast_block_add_stmt(block, next_decl);
        }
        return block;
    }

    return first_decl;
}

int parser_parse_declaration_list(Parser *parser, ASTNode ***out_decls, int *out_count) {
    if (!is_type_token(parser->current.type)) {
        parser_error(parser, "expected type name in declaration");
        return -1;
    }

    Token type_tok = parser_advance(parser);
    ASTType type = token_to_ast_type(type_tok.type);
    token_free(&type_tok);

    int capacity = 4;
    int count = 0;
    ASTNode **decls = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
    if (!decls) {
        fprintf(stderr, "Fatal error: Out of memory in parser_parse_declaration_list\n");
        exit(1);
    }

    while (1) {
        ASTNode *decl = parse_single_init_declarator(parser, type);
        if (!decl) {
            for (int i = 0; i < count; i++) {
                ast_free(decls[i]);
            }
            free(decls);
            return -1;
        }

        if (count >= capacity) {
            capacity *= 2;
            decls = (ASTNode **)realloc(decls, sizeof(ASTNode *) * capacity);
            if (!decls) {
                fprintf(stderr, "Fatal error: Out of memory reallocating declaration list\n");
                exit(1);
            }
        }
        decls[count++] = decl;

        if (parser->current.type == TOKEN_COMMA) {
            Token comma = parser_advance(parser);
            token_free(&comma);
        } else {
            break;
        }
    }

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after declaration but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        for (int i = 0; i < count; i++) {
            ast_free(decls[i]);
        }
        free(decls);
        return -1;
    }

    Token sc = parser_advance(parser);
    token_free(&sc);

    *out_decls = decls;
    *out_count = count;
    return 0;
}

ASTNode *parser_parse_block(Parser *parser) {
    if (parser->current.type != TOKEN_LEFT_BRACE) {
        parser_error(parser, "expected '{' to start block but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }

    int line = parser->current.line;
    int col = parser->current.column;
    Token lb = parser_advance(parser);
    token_free(&lb);

    ASTNode *block = ast_create_block(line, col);

    while (parser->current.type != TOKEN_RIGHT_BRACE && parser->current.type != TOKEN_EOF) {
        if (is_type_token(parser->current.type)) {
            ASTNode **decls = NULL;
            int count = 0;
            if (parser_parse_declaration_list(parser, &decls, &count) != 0) {
                ast_free(block);
                return NULL;
            }
            for (int i = 0; i < count; i++) {
                ast_block_add_stmt(block, decls[i]);
            }
            free(decls);
        } else {
            ASTNode *stmt = parser_parse_statement(parser);
            if (parser->has_error) {
                if (stmt) ast_free(stmt);
                ast_free(block);
                return NULL;
            }
            if (stmt) {
                ast_block_add_stmt(block, stmt);
            }
        }
    }

    if (parser->current.type != TOKEN_RIGHT_BRACE) {
        parser_error(parser, "expected '}' to close block but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(block);
        return NULL;
    }

    Token rb = parser_advance(parser);
    token_free(&rb);
    return block;
}

ASTNode *parser_parse_if_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token if_tok = parser_advance(parser);
    token_free(&if_tok);

    if (parser->current.type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "expected '(' after 'if' but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }
    Token lp = parser_advance(parser);
    token_free(&lp);

    ASTNode *cond = parser_parse_expression(parser);
    if (!cond) return NULL;

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        parser_error(parser, "expected ')' after if condition but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(cond);
        return NULL;
    }
    Token rp = parser_advance(parser);
    token_free(&rp);

    ASTNode *then_branch = parser_parse_statement(parser);
    if (!then_branch) {
        ast_free(cond);
        return NULL;
    }

    ASTNode *else_branch = NULL;
    if (parser->current.type == TOKEN_ELSE) {
        Token else_tok = parser_advance(parser);
        token_free(&else_tok);

        else_branch = parser_parse_statement(parser);
        if (!else_branch) {
            ast_free(cond);
            ast_free(then_branch);
            return NULL;
        }
    }

    return ast_create_if(cond, then_branch, else_branch, line, col);
}

ASTNode *parser_parse_while_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token while_tok = parser_advance(parser);
    token_free(&while_tok);

    if (parser->current.type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "expected '(' after 'while' but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }
    Token lp = parser_advance(parser);
    token_free(&lp);

    ASTNode *cond = parser_parse_expression(parser);
    if (!cond) return NULL;

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        parser_error(parser, "expected ')' after while condition but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(cond);
        return NULL;
    }
    Token rp = parser_advance(parser);
    token_free(&rp);

    ASTNode *body = parser_parse_statement(parser);
    if (!body) {
        ast_free(cond);
        return NULL;
    }

    return ast_create_while(cond, body, line, col);
}

ASTNode *parser_parse_do_while_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token do_tok = parser_advance(parser);
    token_free(&do_tok);

    ASTNode *body = parser_parse_statement(parser);
    if (!body) return NULL;

    if (parser->current.type != TOKEN_WHILE) {
        parser_error(parser, "expected 'while' after do-while body but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(body);
        return NULL;
    }
    Token while_tok = parser_advance(parser);
    token_free(&while_tok);

    if (parser->current.type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "expected '(' after 'while' in do-while statement but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(body);
        return NULL;
    }
    Token lp = parser_advance(parser);
    token_free(&lp);

    ASTNode *cond = parser_parse_expression(parser);
    if (!cond) {
        ast_free(body);
        return NULL;
    }

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        parser_error(parser, "expected ')' after do-while condition but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(body);
        ast_free(cond);
        return NULL;
    }
    Token rp = parser_advance(parser);
    token_free(&rp);

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after do-while statement but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(body);
        ast_free(cond);
        return NULL;
    }
    Token sc = parser_advance(parser);
    token_free(&sc);

    return ast_create_do_while(cond, body, line, col);
}

ASTNode *parser_parse_for_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token for_tok = parser_advance(parser);
    token_free(&for_tok);

    if (parser->current.type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "expected '(' after 'for' but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }
    Token lp = parser_advance(parser);
    token_free(&lp);

    /* for_init */
    ASTNode *init = NULL;
    if (is_type_token(parser->current.type)) {
        init = parser_parse_declaration_no_semicolon(parser);
        if (!init) return NULL;
    } else if (parser->current.type != TOKEN_SEMICOLON) {
        init = parser_parse_expression(parser);
        if (!init) return NULL;
    }

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after for-loop initialization but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        if (init) ast_free(init);
        return NULL;
    }
    Token sc1 = parser_advance(parser);
    token_free(&sc1);

    /* condition_opt */
    ASTNode *cond = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        cond = parser_parse_expression(parser);
        if (!cond) {
            if (init) ast_free(init);
            return NULL;
        }
    }

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after for-loop condition but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        if (init) ast_free(init);
        if (cond) ast_free(cond);
        return NULL;
    }
    Token sc2 = parser_advance(parser);
    token_free(&sc2);

    /* for_update_opt */
    ASTNode *update = NULL;
    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        update = parser_parse_expression(parser);
        if (!update) {
            if (init) ast_free(init);
            if (cond) ast_free(cond);
            return NULL;
        }
    }

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        parser_error(parser, "expected ')' after for-loop update clause but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        if (init) ast_free(init);
        if (cond) ast_free(cond);
        if (update) ast_free(update);
        return NULL;
    }
    Token rp = parser_advance(parser);
    token_free(&rp);

    /* statement */
    ASTNode *body = parser_parse_statement(parser);
    if (!body) {
        if (init) ast_free(init);
        if (cond) ast_free(cond);
        if (update) ast_free(update);
        return NULL;
    }

    return ast_create_for(init, cond, update, body, line, col);
}

ASTNode *parser_parse_return_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token ret_tok = parser_advance(parser);
    token_free(&ret_tok);

    ASTNode *expr = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        expr = parser_parse_expression(parser);
        if (!expr) return NULL;
    }

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after return statement but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        if (expr) ast_free(expr);
        return NULL;
    }
    Token sc = parser_advance(parser);
    token_free(&sc);

    return ast_create_return(expr, line, col);
}

ASTNode *parser_parse_break_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token brk_tok = parser_advance(parser);
    token_free(&brk_tok);

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after break but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }
    Token sc = parser_advance(parser);
    token_free(&sc);

    return ast_create_break(line, col);
}

ASTNode *parser_parse_continue_stmt(Parser *parser) {
    int line = parser->current.line;
    int col = parser->current.column;
    Token cont_tok = parser_advance(parser);
    token_free(&cont_tok);

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after continue but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }
    Token sc = parser_advance(parser);
    token_free(&sc);

    return ast_create_continue(line, col);
}

ASTNode *parser_parse_statement(Parser *parser) {
    TokenType type = parser->current.type;

    if (type == TOKEN_SEMICOLON) {
        Token sc = parser_advance(parser);
        token_free(&sc);
        return NULL;
    }

    if (type == TOKEN_LEFT_BRACE) {
        return parser_parse_block(parser);
    }

    if (type == TOKEN_IF) {
        return parser_parse_if_stmt(parser);
    }

    if (type == TOKEN_WHILE) {
        return parser_parse_while_stmt(parser);
    }

    if (type == TOKEN_DO) {
        return parser_parse_do_while_stmt(parser);
    }

    if (type == TOKEN_FOR) {
        return parser_parse_for_stmt(parser);
    }

    if (type == TOKEN_RETURN) {
        return parser_parse_return_stmt(parser);
    }

    if (type == TOKEN_BREAK) {
        return parser_parse_break_stmt(parser);
    }

    if (type == TOKEN_CONTINUE) {
        return parser_parse_continue_stmt(parser);
    }

    if (is_type_token(type)) {
        ASTNode **decls = NULL;
        int count = 0;
        if (parser_parse_declaration_list(parser, &decls, &count) != 0 || count == 0) {
            return NULL;
        }
        if (count == 1) {
            ASTNode *single = decls[0];
            free(decls);
            return single;
        } else {
            ASTNode *block = ast_create_block(decls[0]->line, decls[0]->column);
            for (int i = 0; i < count; i++) {
                ast_block_add_stmt(block, decls[i]);
            }
            free(decls);
            return block;
        }
    }

    /* expression_statement -> expression ';' */
    ASTNode *expr = parser_parse_expression(parser);
    if (!expr) return NULL;

    if (parser->current.type != TOKEN_SEMICOLON) {
        parser_error(parser, "expected ';' after expression statement but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(expr);
        return NULL;
    }
    Token sc = parser_advance(parser);
    token_free(&sc);
    return expr;
}

/*
 * =========================================================================
 * Stage 6: Function Definitions and Complete Program Parsing
 * =========================================================================
 */

/*
 * function_definition
 *  -> 'int' 'main' '(' ')' block
 *   | type identifier '(' parameter_list_opt ')' block
 */
ASTNode *parser_parse_function_definition(Parser *parser) {
    if (!is_type_token(parser->current.type)) {
        parser_error(parser, "expected return type for function definition but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }

    Token type_tok = parser_advance(parser);
    ASTType return_type = token_to_ast_type(type_tok.type);
    int line = type_tok.line;
    int col = type_tok.column;
    token_free(&type_tok);

    if (parser->current.type != TOKEN_IDENTIFIER && parser->current.type != TOKEN_MAIN) {
        parser_error(parser, "expected function name but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        return NULL;
    }

    Token name_tok = parser_advance(parser);
    const char *func_name = name_tok.lexeme;

    if (parser->current.type != TOKEN_LEFT_PAREN) {
        parser_error(parser, "expected '(' after function name but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        token_free(&name_tok);
        return NULL;
    }
    Token lp = parser_advance(parser);
    token_free(&lp);

    ASTNode *func = ast_create_function(func_name, return_type, line, col);
    token_free(&name_tok);

    /* Check for main() vs regular function */
    if (func->data.function.name && strcmp(func->data.function.name, "main") == 0) {
        if (parser->current.type != TOKEN_RIGHT_PAREN) {
            parser_error(parser, "expected ')' for parameterless main function but found '%s'",
                         parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            ast_free(func);
            return NULL;
        }
        Token rp = parser_advance(parser);
        token_free(&rp);
    } else {
        /* parameter_list_opt */
        if (parser->current.type == TOKEN_RIGHT_PAREN) {
            Token rp = parser_advance(parser);
            token_free(&rp);
        } else {
            while (1) {
                if (!is_type_token(parser->current.type)) {
                    parser_error(parser, "expected parameter type but found '%s'",
                                 parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                    ast_free(func);
                    return NULL;
                }
                Token ptype_tok = parser_advance(parser);
                ASTType ptype = token_to_ast_type(ptype_tok.type);
                token_free(&ptype_tok);

                if (parser->current.type != TOKEN_IDENTIFIER) {
                    parser_error(parser, "expected parameter name but found '%s'",
                                 parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                    ast_free(func);
                    return NULL;
                }
                Token pname_tok = parser_advance(parser);
                ast_function_add_param(func, ptype, pname_tok.lexeme);
                token_free(&pname_tok);

                if (parser->current.type == TOKEN_COMMA) {
                    Token comma = parser_advance(parser);
                    token_free(&comma);
                } else {
                    break;
                }
            }

            if (parser->current.type != TOKEN_RIGHT_PAREN) {
                parser_error(parser, "expected ')' after parameter list but found '%s'",
                             parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
                ast_free(func);
                return NULL;
            }
            Token rp = parser_advance(parser);
            token_free(&rp);
        }
    }

    /* Function body: block '{' statement_list '}' */
    if (parser->current.type != TOKEN_LEFT_BRACE) {
        parser_error(parser, "expected '{' for function body but found '%s'",
                     parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
        ast_free(func);
        return NULL;
    }

    ASTNode *body = parser_parse_block(parser);
    if (!body) {
        ast_free(func);
        return NULL;
    }

    ast_function_set_body(func, body);
    return func;
}

/*
 * program -> external_declaration*
 * external_declaration -> function_definition | declaration
 */
ASTNode *parser_parse_program(Parser *parser) {
    if (!parser) return NULL;

    int line = parser->current.line;
    int col = parser->current.column;
    ASTNode *prog = ast_create_program(line, col);

    while (parser->current.type != TOKEN_EOF) {
        if (!is_type_token(parser->current.type)) {
            if (parser->current.type == TOKEN_MAIN) {
                parser_error(parser, "expected return type for 'main' function");
            } else {
                parser_error(parser, "expected declaration or function definition at top level but found '%s'",
                             parser->current.lexeme ? parser->current.lexeme : token_type_name(parser->current.type));
            }
            ast_free(prog);
            return NULL;
        }

        /*
         * Disambiguate function_definition vs global declaration:
         * parser->current is type
         * parser->next is identifier or main
         * parser->next2 is the token following the identifier.
         * If parser->next2 is '(', it is a function definition!
         * Otherwise ('=', '[', ',', ';'), it is a declaration.
         */
        if ((parser->next.type == TOKEN_IDENTIFIER || parser->next.type == TOKEN_MAIN) &&
            parser->next2.type == TOKEN_LEFT_PAREN) {
            ASTNode *func = parser_parse_function_definition(parser);
            if (!func || parser->has_error) {
                if (func) ast_free(func);
                ast_free(prog);
                return NULL;
            }
            ast_program_add_decl(prog, func);
        } else {
            ASTNode **decls = NULL;
            int count = 0;
            if (parser_parse_declaration_list(parser, &decls, &count) != 0 || parser->has_error) {
                ast_free(prog);
                return NULL;
            }
            for (int i = 0; i < count; i++) {
                ast_program_add_decl(prog, decls[i]);
            }
            free(decls);
        }
    }

    if (parser->current.type != TOKEN_EOF) {
        parser_error(parser, "unexpected extra tokens after program completion");
        ast_free(prog);
        return NULL;
    }

    if (parser->has_error) {
        ast_free(prog);
        return NULL;
    }

    return prog;
}
