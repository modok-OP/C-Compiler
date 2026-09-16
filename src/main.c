#include <stdio.h>
#include <stdlib.h>
#include "token.h"

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

    /* Source file supplied - acknowledge without compiling */
    printf("Source file: %s\n", argv[1]);
    printf("Front-end foundation initialized successfully.\n");
    printf("No compilation performed (Foundation stage).\n");

    /* 5: Exit cleanly */
    return 0;
}
