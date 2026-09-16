#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbol_table.h"
#include "semantic.h"

int main(int argc, char *argv[]) {
    /* 1 & 2: Compiler banner */
    printf("========================================\n");
    printf("        C Compiler Front-End\n");
    printf("  Frozen Specification Version 1.0\n");
    printf("========================================\n\n");

    /* Argument parsing */
    int dump_tokens = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0 || strcmp(argv[i], "-v") == 0) {
            dump_tokens = 1;
        } else if (argv[i][0] != '-' && filepath == NULL) {
            filepath = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown or unexpected argument '%s'\n", argv[i]);
            fprintf(stderr, "Usage: %s [--tokens] <source_file.c>\n", argv[0]);
            return 1;
        }
    }

    if (!filepath) {
        fprintf(stderr, "Error: No input source file provided.\n");
        fprintf(stderr, "Usage: %s [--tokens] <source_file.c>\n", argv[0]);
        fprintf(stderr, "Example: %s examples/test.c\n", argv[0]);
        return 1;
    }

    printf("Source file: %s\n", filepath);

    /* ----------------------------------------------------
     * STAGE 1: Lexical Analysis
     * ---------------------------------------------------- */
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
        if (dump_tokens) {
            token_print(&token);
        }
        token_count++;
        if (token.type == TOKEN_LEXICAL_ERROR) {
            error_count++;
        }
        token_free(&token);
    } while (token.type != TOKEN_EOF);

    printf("--- Lexical Analysis Complete ---\n");
    printf("Total tokens: %d, Lexical errors: %d\n", token_count, error_count);

    lexer_destroy(lexer);

    if (error_count > 0) {
        fprintf(stderr, "\nLexical Analysis Failed: %d error(s) found. Halting.\n", error_count);
        return 1;
    }

    /* ----------------------------------------------------
     * STAGE 2: Syntax Analysis (Parser)
     * ---------------------------------------------------- */
    printf("\n--- Beginning Syntax Analysis ---\n");
    Lexer *parse_lexer = lexer_create_from_file(filepath);
    if (!parse_lexer) {
        fprintf(stderr, "Error: Could not re-open source file for parsing.\n");
        return 1;
    }

    Parser *parser = parser_create(parse_lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        fprintf(stderr, "\nSyntax Analysis Failed: %d error(s) found. Halting.\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(parse_lexer);
        return 1;
    }
    printf("--- Syntax Analysis Complete: Valid AST Constructed ---\n");

    /* ----------------------------------------------------
     * STAGE 3: Semantic Analysis
     * ---------------------------------------------------- */
    printf("\n--- Beginning Semantic Analysis ---\n");
    SemanticContext *sem_ctx = semantic_context_create();
    int sem_errors = semantic_analyze(sem_ctx, ast);

    int exit_code = 0;
    if (sem_errors == 0) {
        printf("--- Semantic Analysis Complete: 0 Errors Found ---\n");
        printf("\n========================================\n");
        printf("Compilation Successful: %s validated.\n", filepath);
        printf("========================================\n");
        exit_code = 0;
    } else {
        fprintf(stderr, "\nSemantic Analysis Failed: %d error(s) found.\n", sem_errors);
        exit_code = 1;
    }

    /* Clean up all allocated compiler resources */
    semantic_context_destroy(sem_ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(parse_lexer);

    return exit_code;
}
