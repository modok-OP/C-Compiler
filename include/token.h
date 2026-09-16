#ifndef TOKEN_H
#define TOKEN_H

/*
 * Token definitions for the C Compiler front end.
 * Based on the frozen C Compiler Front-End Language & Semantic Specification v1.0.
 */

typedef enum {
    /* Keywords (Section 6) */
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_VOID,
    TOKEN_MAIN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DO,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    /* Identifiers (Section 7) */
    TOKEN_IDENTIFIER,

    /* Literals (Section 8) */
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,

    /* Operators - Arithmetic (Section 10) */
    TOKEN_PLUS,             /* + */
    TOKEN_MINUS,            /* - */
    TOKEN_MULTIPLY,         /* * */
    TOKEN_DIVIDE,           /* / */
    TOKEN_MODULO,           /* % */

    /* Operators - Relational and Equality (Section 10) */
    TOKEN_LESS,             /* < */
    TOKEN_GREATER,          /* > */
    TOKEN_LESS_EQUAL,       /* <= */
    TOKEN_GREATER_EQUAL,    /* >= */
    TOKEN_EQUAL,            /* == */
    TOKEN_NOT_EQUAL,        /* != */

    /* Operators - Logical (Section 10) */
    TOKEN_LOGICAL_AND,      /* && */
    TOKEN_LOGICAL_OR,       /* || */
    TOKEN_LOGICAL_NOT,      /* ! */

    /* Operators - Assignment (Section 10) */
    TOKEN_ASSIGN,           /* = */
    TOKEN_PLUS_ASSIGN,      /* += */
    TOKEN_MINUS_ASSIGN,     /* -= */
    TOKEN_MULTIPLY_ASSIGN,  /* *= */
    TOKEN_DIVIDE_ASSIGN,    /* /= */
    TOKEN_MODULO_ASSIGN,    /* %= */

    /* Operators - Increment / Decrement (Section 10) */
    TOKEN_INCREMENT,        /* ++ */
    TOKEN_DECREMENT,        /* -- */

    /* Delimiters and Special Symbols (Section 11) */
    TOKEN_LEFT_PAREN,       /* ( */
    TOKEN_RIGHT_PAREN,      /* ) */
    TOKEN_LEFT_BRACE,       /* { */
    TOKEN_RIGHT_BRACE,      /* } */
    TOKEN_LEFT_BRACKET,     /* [ */
    TOKEN_RIGHT_BRACKET,    /* ] */
    TOKEN_SEMICOLON,        /* ; */
    TOKEN_COMMA,            /* , */
    TOKEN_AMPERSAND,        /* & (scanf destination notation only) */

    /* Special Tokens */
    TOKEN_EOF,              /* End of input stream */
    TOKEN_LEXICAL_ERROR     /* Lexical error / unrecognized token */
} TokenType;

/*
 * Token structure representing a lexical token with source location.
 */
typedef struct {
    TokenType type;
    const char *lexeme;     /* Lexeme string representation */
    int line;               /* 1-based source line number */
    int column;             /* 1-based source column number */
} Token;

/*
 * Helper functions
 */

/* Returns human-readable string representation of a token type enum */
const char *token_type_name(TokenType type);

/* Constructs and returns a Token value */
Token token_create(TokenType type, const char *lexeme, int line, int column);

/* Prints token details in readable diagnostic format */
void token_print(const Token *token);

#endif /* TOKEN_H */
