#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
    /* 1 & 2: Compiler program startup and banner */
    printf("========================================\n");
    printf("        C Compiler Front-End\n");
    printf("  Frozen Specification Version 1.0\n");
    printf("========================================\n\n");

    /* 3 & 4: Verify whether a source filename argument was supplied */
    if (argc < 2) {
        fprintf(stderr, "Error: No input source file provided.\n");
        fprintf(stderr, "Usage: %s <source_file.c>\n", argv[0]);
        fprintf(stderr, "Example: %s examples/test.c\n", argv[0]);
        return 1;
    }

    const char *filepath = argv[1];
    printf("Source file: %s\n", filepath);
    printf("--- Beginning Lexical Analysis ---\n");

    Lexer *lexer = lexer_create_from_file(filepath);
    if (!lexer) {
        fprintf(stderr, "Error: Could not open or read source file '%s'.\n", filepath);
        return 1;
    }

    int token_count = 0;
    int error_count = 0;
    Token token;

    do {
        token = lexer_next_token(lexer);
        token_print(&token);
        token_count++;
        if (token.type == TOKEN_LEXICAL_ERROR) {
            error_count++;
        }
        token_free(&token);
    } while (token.type != TOKEN_EOF);

    printf("--- Lexical Analysis Complete ---\n");
    printf("Total tokens: %d, Lexical errors: %d\n", token_count, error_count);

    lexer_destroy(lexer);
    return (error_count == 0) ? 0 : 1;
}
