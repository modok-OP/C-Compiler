#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *token_type_name(TokenType type) {
    switch (type) {
        /* Keywords */
        case TOKEN_INT:             return "TOKEN_INT";
        case TOKEN_FLOAT:           return "TOKEN_FLOAT";
        case TOKEN_CHAR:            return "TOKEN_CHAR";
        case TOKEN_VOID:            return "TOKEN_VOID";
        case TOKEN_MAIN:            return "TOKEN_MAIN";
        case TOKEN_IF:              return "TOKEN_IF";
        case TOKEN_ELSE:            return "TOKEN_ELSE";
        case TOKEN_WHILE:           return "TOKEN_WHILE";
        case TOKEN_FOR:             return "TOKEN_FOR";
        case TOKEN_DO:              return "TOKEN_DO";
        case TOKEN_RETURN:          return "TOKEN_RETURN";
        case TOKEN_BREAK:           return "TOKEN_BREAK";
        case TOKEN_CONTINUE:        return "TOKEN_CONTINUE";

        /* Identifiers */
        case TOKEN_IDENTIFIER:      return "TOKEN_IDENTIFIER";

        /* Literals */
        case TOKEN_INT_LITERAL:     return "TOKEN_INT_LITERAL";
        case TOKEN_FLOAT_LITERAL:   return "TOKEN_FLOAT_LITERAL";
        case TOKEN_CHAR_LITERAL:    return "TOKEN_CHAR_LITERAL";
        case TOKEN_STRING_LITERAL:  return "TOKEN_STRING_LITERAL";

        /* Operators - Arithmetic */
        case TOKEN_PLUS:            return "TOKEN_PLUS";
        case TOKEN_MINUS:           return "TOKEN_MINUS";
        case TOKEN_MULTIPLY:        return "TOKEN_MULTIPLY";
        case TOKEN_DIVIDE:          return "TOKEN_DIVIDE";
        case TOKEN_MODULO:          return "TOKEN_MODULO";

        /* Operators - Relational and Equality */
        case TOKEN_LESS:            return "TOKEN_LESS";
        case TOKEN_GREATER:         return "TOKEN_GREATER";
        case TOKEN_LESS_EQUAL:      return "TOKEN_LESS_EQUAL";
        case TOKEN_GREATER_EQUAL:   return "TOKEN_GREATER_EQUAL";
        case TOKEN_EQUAL:           return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL:       return "TOKEN_NOT_EQUAL";

        /* Operators - Logical */
        case TOKEN_LOGICAL_AND:     return "TOKEN_LOGICAL_AND";
        case TOKEN_LOGICAL_OR:      return "TOKEN_LOGICAL_OR";
        case TOKEN_LOGICAL_NOT:     return "TOKEN_LOGICAL_NOT";

        /* Operators - Assignment */
        case TOKEN_ASSIGN:          return "TOKEN_ASSIGN";
        case TOKEN_PLUS_ASSIGN:     return "TOKEN_PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN:    return "TOKEN_MINUS_ASSIGN";
        case TOKEN_MULTIPLY_ASSIGN: return "TOKEN_MULTIPLY_ASSIGN";
        case TOKEN_DIVIDE_ASSIGN:   return "TOKEN_DIVIDE_ASSIGN";
        case TOKEN_MODULO_ASSIGN:   return "TOKEN_MODULO_ASSIGN";

        /* Operators - Increment / Decrement */
        case TOKEN_INCREMENT:       return "TOKEN_INCREMENT";
        case TOKEN_DECREMENT:       return "TOKEN_DECREMENT";

        /* Delimiters and Special Symbols */
        case TOKEN_LEFT_PAREN:      return "TOKEN_LEFT_PAREN";
        case TOKEN_RIGHT_PAREN:     return "TOKEN_RIGHT_PAREN";
        case TOKEN_LEFT_BRACE:      return "TOKEN_LEFT_BRACE";
        case TOKEN_RIGHT_BRACE:     return "TOKEN_RIGHT_BRACE";
        case TOKEN_LEFT_BRACKET:    return "TOKEN_LEFT_BRACKET";
        case TOKEN_RIGHT_BRACKET:   return "TOKEN_RIGHT_BRACKET";
        case TOKEN_SEMICOLON:       return "TOKEN_SEMICOLON";
        case TOKEN_COMMA:           return "TOKEN_COMMA";
        case TOKEN_AMPERSAND:       return "TOKEN_AMPERSAND";

        /* Special Tokens */
        case TOKEN_EOF:             return "TOKEN_EOF";
        case TOKEN_LEXICAL_ERROR:   return "TOKEN_LEXICAL_ERROR";

        default:                    return "TOKEN_UNKNOWN";
    }
}

static char *allocate_string(const char *start, size_t len) {
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Fatal error: Out of memory while allocating token lexeme.\n");
        exit(1);
    }
    if (len > 0 && start) {
        memcpy(copy, start, len);
    }
    copy[len] = '\0';
    return copy;
}

Token token_create(TokenType type, const char *lexeme, int line, int column) {
    Token token;
    token.type = type;
    token.lexeme = lexeme ? allocate_string(lexeme, strlen(lexeme)) : allocate_string("", 0);
    token.line = line;
    token.column = column;
    return token;
}

Token token_create_len(TokenType type, const char *start, int len, int line, int column) {
    Token token;
    token.type = type;
    token.lexeme = allocate_string(start, len > 0 ? (size_t)len : 0);
    token.line = line;
    token.column = column;
    return token;
}

void token_free(Token *token) {
    if (token && token->lexeme) {
        free((void *)token->lexeme);
        token->lexeme = NULL;
    }
}

void token_print(const Token *token) {
    if (!token) return;
    printf("Token(%s, \"%s\", line: %d, col: %d)\n",
           token_type_name(token->type),
           token->lexeme ? token->lexeme : "",
           token->line,
           token->column);
}
