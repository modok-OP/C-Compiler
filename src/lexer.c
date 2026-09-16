#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Lexer *lexer_create_from_file(const char *filename) {
    if (!filename) return NULL;

    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return NULL;
    }

    rewind(file);

    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)file_size, file);
    buffer[read_bytes] = '\0';
    fclose(file);

    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (!lexer) {
        free(buffer);
        return NULL;
    }

    lexer->source = buffer;
    lexer->source_len = read_bytes;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename;
    lexer->error_count = 0;
    lexer->owns_source = 1;

    return lexer;
}

Lexer *lexer_create_from_string(const char *source, const char *filename) {
    if (!source) return NULL;

    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (!lexer) return NULL;

    size_t len = strlen(source);
    char *buffer = (char *)malloc(len + 1);
    if (!buffer) {
        free(lexer);
        return NULL;
    }
    memcpy(buffer, source, len + 1);

    lexer->source = buffer;
    lexer->source_len = len;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename ? filename : "<string>";
    lexer->error_count = 0;
    lexer->owns_source = 1;

    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    if (lexer->owns_source && lexer->source) {
        free(lexer->source);
        lexer->source = NULL;
    }
    free(lexer);
}

static char lexer_peek(const Lexer *lexer) {
    if (lexer->cursor >= lexer->source_len) {
        return '\0';
    }
    return lexer->source[lexer->cursor];
}

static char lexer_peek_ahead(const Lexer *lexer, size_t offset) {
    if (lexer->cursor + offset >= lexer->source_len) {
        return '\0';
    }
    return lexer->source[lexer->cursor + offset];
}

static char lexer_advance(Lexer *lexer) {
    if (lexer->cursor >= lexer->source_len) {
        return '\0';
    }
    char c = lexer->source[lexer->cursor++];
    if (c == '\r') {
        if (lexer->cursor < lexer->source_len && lexer->source[lexer->cursor] == '\n') {
            lexer->cursor++;
        }
        lexer->line++;
        lexer->column = 1;
        return '\n';
    } else if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
        return '\n';
    } else {
        lexer->column++;
        return c;
    }
}

static TokenType check_keyword(const char *text, int len) {
    switch (len) {
        case 2:
            if (memcmp(text, "if", 2) == 0) return TOKEN_IF;
            if (memcmp(text, "do", 2) == 0) return TOKEN_DO;
            break;
        case 3:
            if (memcmp(text, "int", 3) == 0) return TOKEN_INT;
            if (memcmp(text, "for", 3) == 0) return TOKEN_FOR;
            break;
        case 4:
            if (memcmp(text, "char", 4) == 0) return TOKEN_CHAR;
            if (memcmp(text, "void", 4) == 0) return TOKEN_VOID;
            if (memcmp(text, "main", 4) == 0) return TOKEN_MAIN;
            if (memcmp(text, "else", 4) == 0) return TOKEN_ELSE;
            break;
        case 5:
            if (memcmp(text, "float", 5) == 0) return TOKEN_FLOAT;
            if (memcmp(text, "while", 5) == 0) return TOKEN_WHILE;
            if (memcmp(text, "break", 5) == 0) return TOKEN_BREAK;
            break;
        case 6:
            if (memcmp(text, "return", 6) == 0) return TOKEN_RETURN;
            break;
        case 8:
            if (memcmp(text, "continue", 8) == 0) return TOKEN_CONTINUE;
            break;
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_next_token(Lexer *lexer) {
    if (!lexer) {
        return token_create(TOKEN_EOF, "", 0, 0);
    }

    /* Skip whitespace and comments */
    while (1) {
        char c = lexer_peek(lexer);
        if (c == '\0') {
            break;
        }

        /* Whitespace */
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            lexer_advance(lexer);
            continue;
        }

        /* Single-line comment: // ... */
        if (c == '/' && lexer_peek_ahead(lexer, 1) == '/') {
            lexer_advance(lexer); /* / */
            lexer_advance(lexer); /* / */
            while (lexer_peek(lexer) != '\0' && lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\r') {
                lexer_advance(lexer);
            }
            continue;
        }

        /* Multi-line comment: / * ... * / */
        if (c == '/' && lexer_peek_ahead(lexer, 1) == '*') {
            int comment_line = lexer->line;
            int comment_col = lexer->column;
            lexer_advance(lexer); /* / */
            lexer_advance(lexer); /* * */
            int terminated = 0;
            while (lexer_peek(lexer) != '\0') {
                if (lexer_peek(lexer) == '*' && lexer_peek_ahead(lexer, 1) == '/') {
                    lexer_advance(lexer); /* * */
                    lexer_advance(lexer); /* / */
                    terminated = 1;
                    break;
                }
                lexer_advance(lexer);
            }
            if (!terminated) {
                lexer->error_count++;
                return token_create(TOKEN_LEXICAL_ERROR, "Unterminated multi-line comment", comment_line, comment_col);
            }
            continue;
        }

        break;
    }

    char c = lexer_peek(lexer);
    if (c == '\0') {
        return token_create(TOKEN_EOF, "", lexer->line, lexer->column);
    }

    int start_line = lexer->line;
    int start_col = lexer->column;
    size_t start_idx = lexer->cursor;

    /* Identifiers and Keywords: (letter | _) (letter | digit | _)* */
    if (isalpha((unsigned char)c) || c == '_') {
        lexer_advance(lexer);
        while (isalnum((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
            lexer_advance(lexer);
        }
        int len = (int)(lexer->cursor - start_idx);
        TokenType type = check_keyword(&lexer->source[start_idx], len);
        return token_create_len(type, &lexer->source[start_idx], len, start_line, start_col);
    }

    /* Integer and Float Literals */
    if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)lexer_peek(lexer))) {
            lexer_advance(lexer);
        }

        int is_float = 0;
        int is_error = 0;

        if (lexer_peek(lexer) == '.') {
            if (isdigit((unsigned char)lexer_peek_ahead(lexer, 1))) {
                is_float = 1;
                lexer_advance(lexer); /* consume '.' */
                while (isdigit((unsigned char)lexer_peek(lexer))) {
                    lexer_advance(lexer);
                }
            } else {
                /* '.' not followed by digits, e.g. '10.' */
                is_error = 1;
                lexer_advance(lexer); /* consume '.' */
            }
        }

        /* Check for attached letters or underscore (e.g. 1x, 2student, 3.14f) */
        if (isalpha((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
            is_error = 1;
            while (isalnum((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
                lexer_advance(lexer);
            }
        }

        int len = (int)(lexer->cursor - start_idx);
        if (is_error) {
            lexer->error_count++;
            return token_create_len(TOKEN_LEXICAL_ERROR, &lexer->source[start_idx], len, start_line, start_col);
        }

        if (is_float) {
            return token_create_len(TOKEN_FLOAT_LITERAL, &lexer->source[start_idx], len, start_line, start_col);
        } else {
            return token_create_len(TOKEN_INT_LITERAL, &lexer->source[start_idx], len, start_line, start_col);
        }
    }

    /* Character Literals: 'A', '\n', etc. */
    if (c == '\'') {
        lexer_advance(lexer); /* consume opening '\'' */

        char next = lexer_peek(lexer);
        if (next == '\'') {
            /* Empty character literal '' */
            lexer_advance(lexer);
            lexer->error_count++;
            int len = (int)(lexer->cursor - start_idx);
            return token_create_len(TOKEN_LEXICAL_ERROR, &lexer->source[start_idx], len, start_line, start_col);
        }

        if (next == '\0' || next == '\n' || next == '\r') {
            /* Unterminated character literal */
            lexer->error_count++;
            int len = (int)(lexer->cursor - start_idx);
            return token_create_len(TOKEN_LEXICAL_ERROR, &lexer->source[start_idx], len, start_line, start_col);
        }

        int is_error = 0;
        if (next == '\\') {
            lexer_advance(lexer); /* consume '\\' */
            char esc = lexer_peek(lexer);
            if (esc == 'n' || esc == 't' || esc == '\\' || esc == '\'' || esc == '\"') {
                lexer_advance(lexer);
            } else {
                is_error = 1;
                if (esc != '\0' && esc != '\n' && esc != '\r') {
                    lexer_advance(lexer);
                }
            }
        } else {
            lexer_advance(lexer); /* consume normal char */
        }

        if (lexer_peek(lexer) == '\'') {
            lexer_advance(lexer); /* consume closing '\'' */
        } else {
            is_error = 1;
            while (lexer_peek(lexer) != '\'' && lexer_peek(lexer) != '\n' &&
                   lexer_peek(lexer) != '\r' && lexer_peek(lexer) != '\0') {
                lexer_advance(lexer);
            }
            if (lexer_peek(lexer) == '\'') {
                lexer_advance(lexer);
            }
        }

        int len = (int)(lexer->cursor - start_idx);
        if (is_error) {
            lexer->error_count++;
            return token_create_len(TOKEN_LEXICAL_ERROR, &lexer->source[start_idx], len, start_line, start_col);
        }
        return token_create_len(TOKEN_CHAR_LITERAL, &lexer->source[start_idx], len, start_line, start_col);
    }

    /* String Literals: "Hello\n" */
    if (c == '\"') {
        lexer_advance(lexer); /* consume opening '\"' */

        int is_error = 0;
        while (1) {
            char sc = lexer_peek(lexer);
            if (sc == '\0' || sc == '\n' || sc == '\r') {
                /* Unterminated string literal */
                is_error = 1;
                break;
            }

            if (sc == '\"') {
                lexer_advance(lexer); /* consume closing '\"' */
                break;
            }

            if (sc == '\\') {
                lexer_advance(lexer); /* consume '\\' */
                char esc = lexer_peek(lexer);
                if (esc == '\0' || esc == '\n' || esc == '\r') {
                    is_error = 1;
                    break;
                }
                if (esc == 'n' || esc == 't' || esc == '\\' || esc == '\"' || esc == '\'') {
                    lexer_advance(lexer);
                } else {
                    is_error = 1;
                    lexer_advance(lexer);
                }
            } else {
                lexer_advance(lexer);
            }
        }

        int len = (int)(lexer->cursor - start_idx);
        if (is_error) {
            lexer->error_count++;
            return token_create_len(TOKEN_LEXICAL_ERROR, &lexer->source[start_idx], len, start_line, start_col);
        }
        return token_create_len(TOKEN_STRING_LITERAL, &lexer->source[start_idx], len, start_line, start_col);
    }

    /* Operators, Delimiters, and Special Symbols */
    switch (c) {
        case '+':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '+') {
                lexer_advance(lexer);
                return token_create(TOKEN_INCREMENT, "++", start_line, start_col);
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_PLUS_ASSIGN, "+=", start_line, start_col);
            }
            return token_create(TOKEN_PLUS, "+", start_line, start_col);

        case '-':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '-') {
                lexer_advance(lexer);
                return token_create(TOKEN_DECREMENT, "--", start_line, start_col);
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_MINUS_ASSIGN, "-=", start_line, start_col);
            }
            return token_create(TOKEN_MINUS, "-", start_line, start_col);

        case '*':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_MULTIPLY_ASSIGN, "*=", start_line, start_col);
            }
            return token_create(TOKEN_MULTIPLY, "*", start_line, start_col);

        case '/':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_DIVIDE_ASSIGN, "/=", start_line, start_col);
            }
            return token_create(TOKEN_DIVIDE, "/", start_line, start_col);

        case '%':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_MODULO_ASSIGN, "%=", start_line, start_col);
            }
            return token_create(TOKEN_MODULO, "%", start_line, start_col);

        case '<':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_LESS_EQUAL, "<=", start_line, start_col);
            }
            return token_create(TOKEN_LESS, "<", start_line, start_col);

        case '>':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_GREATER_EQUAL, ">=", start_line, start_col);
            }
            return token_create(TOKEN_GREATER, ">", start_line, start_col);

        case '=':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_EQUAL, "==", start_line, start_col);
            }
            return token_create(TOKEN_ASSIGN, "=", start_line, start_col);

        case '!':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_NOT_EQUAL, "!=", start_line, start_col);
            }
            return token_create(TOKEN_LOGICAL_NOT, "!", start_line, start_col);

        case '&':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '&') {
                lexer_advance(lexer);
                return token_create(TOKEN_LOGICAL_AND, "&&", start_line, start_col);
            }
            return token_create(TOKEN_AMPERSAND, "&", start_line, start_col);

        case '|':
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '|') {
                lexer_advance(lexer);
                return token_create(TOKEN_LOGICAL_OR, "||", start_line, start_col);
            }
            /* Standalone '|' is unsupported bitwise OR in the baseline */
            lexer->error_count++;
            return token_create(TOKEN_LEXICAL_ERROR, "|", start_line, start_col);

        case '(':
            lexer_advance(lexer);
            return token_create(TOKEN_LEFT_PAREN, "(", start_line, start_col);

        case ')':
            lexer_advance(lexer);
            return token_create(TOKEN_RIGHT_PAREN, ")", start_line, start_col);

        case '{':
            lexer_advance(lexer);
            return token_create(TOKEN_LEFT_BRACE, "{", start_line, start_col);

        case '}':
            lexer_advance(lexer);
            return token_create(TOKEN_RIGHT_BRACE, "}", start_line, start_col);

        case '[':
            lexer_advance(lexer);
            return token_create(TOKEN_LEFT_BRACKET, "[", start_line, start_col);

        case ']':
            lexer_advance(lexer);
            return token_create(TOKEN_RIGHT_BRACKET, "]", start_line, start_col);

        case ';':
            lexer_advance(lexer);
            return token_create(TOKEN_SEMICOLON, ";", start_line, start_col);

        case ',':
            lexer_advance(lexer);
            return token_create(TOKEN_COMMA, ",", start_line, start_col);

        default: {
            char invalid_char = lexer_advance(lexer);
            char err_buf[2] = { invalid_char, '\0' };
            lexer->error_count++;
            return token_create(TOKEN_LEXICAL_ERROR, err_buf, start_line, start_col);
        }
    }
}
