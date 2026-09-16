#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include "token.h"

/*
 * Lexer state representing the source buffer, scanning cursor,
 * 1-based source coordinates, and diagnostic counters.
 */
typedef struct {
    char *source;             /* Null-terminated source code buffer */
    size_t source_len;        /* Total length of source buffer */
    size_t cursor;            /* Current byte index in source */
    int line;                 /* Current line number (1-based) */
    int column;               /* Current column number (1-based) */
    const char *filename;     /* Source filename for diagnostics */
    int error_count;          /* Total count of lexical errors */
    int owns_source;          /* 1 if lexer allocated source buffer, 0 otherwise */
} Lexer;

/*
 * Creates a new Lexer instance by reading a source file from disk.
 * Returns NULL if the file cannot be opened or read.
 */
Lexer *lexer_create_from_file(const char *filename);

/*
 * Creates a new Lexer instance from an in-memory null-terminated string.
 */
Lexer *lexer_create_from_string(const char *source, const char *filename);

/*
 * Frees resources associated with the Lexer instance.
 */
void lexer_destroy(Lexer *lexer);

/*
 * Scans and returns the next token from the source stream.
 * Automatically skips whitespace and comments.
 * Returns TOKEN_EOF when reaching end of input.
 * Returns TOKEN_LEXICAL_ERROR on lexical violations without crashing.
 */
Token lexer_next_token(Lexer *lexer);

#endif /* LEXER_H */
