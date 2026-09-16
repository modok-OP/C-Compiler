#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbol_table.h"
#include "semantic.h"

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

    if (error_count > 0) {
        return 1;
    }

    /* 5: Syntax Analysis */
    printf("\n--- Beginning Syntax Analysis ---\n");
    Lexer *parse_lexer = lexer_create_from_file(filepath);
    Parser *parser = parser_create(parse_lexer);
    ASTNode *ast = parser_parse_program(parser);

    if (!ast || parser->error_count > 0) {
        fprintf(stderr, "Syntax Analysis Failed: %d error(s)\n", parser->error_count);
        if (ast) ast_free(ast);
        parser_destroy(parser);
        lexer_destroy(parse_lexer);
        return 1;
    }
    printf("--- Syntax Analysis Complete: Valid AST Constructed ---\n");

    /* 6: Semantic Analysis */
    printf("\n--- Beginning Semantic Analysis ---\n");
    SemanticContext *sem_ctx = semantic_context_create();
    int sem_errors = semantic_analyze(sem_ctx, ast);

    if (sem_errors == 0) {
        printf("--- Semantic Analysis Complete: 0 Errors Found ---\n");
        printf("\n========================================\n");
        printf("Compilation Successful: %s validated.\n", filepath);
        printf("========================================\n");
    } else {
        fprintf(stderr, "Semantic Analysis Failed: %d error(s)\n", sem_errors);
    }

    semantic_context_destroy(sem_ctx);
    ast_free(ast);
    parser_destroy(parser);
    lexer_destroy(parse_lexer);

    return (sem_errors == 0) ? 0 : 1;
}
