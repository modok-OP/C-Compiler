# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
TARGET = compiler.exe
TEST_AST = $(BUILDDIR)/test_ast.exe
TEST_EXPR = $(BUILDDIR)/test_expressions.exe
TEST_STMT = $(BUILDDIR)/test_statements.exe
TEST_PROG = $(BUILDDIR)/test_program.exe
TEST_SYM = $(BUILDDIR)/test_symbol_table.exe
TEST_SCOPE = $(BUILDDIR)/test_scope_analysis.exe
TEST_TYPE = $(BUILDDIR)/test_type_system.exe
TEST_SEM = $(BUILDDIR)/test_semantic_analyzer.exe
TEST_FULL = $(BUILDDIR)/test_full_semantic.exe
TEST_IO = $(BUILDDIR)/test_io_semantics.exe


# Sources, Headers, and Objects
SRCS = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(INCDIR)/*.h)
OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run AST test
test_ast: $(TEST_AST)
	$(TEST_AST)

$(TEST_AST): tests/parser/test_ast.c $(BUILDDIR)/ast.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Expression Parser test
test_expressions: $(TEST_EXPR)
	$(TEST_EXPR)

$(TEST_EXPR): tests/parser/test_expressions.c $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Statements Parser test
test_statements: $(TEST_STMT)
	$(TEST_STMT)

$(TEST_STMT): tests/parser/test_statements.c $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Program Parser test
test_program: $(TEST_PROG)
	$(TEST_PROG)

$(TEST_PROG): tests/parser/test_program.c $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Symbol Table test
test_symbol_table: $(TEST_SYM)
	$(TEST_SYM)

$(TEST_SYM): tests/semantic/test_symbol_table.c $(BUILDDIR)/symbol_table.o $(BUILDDIR)/ast.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Scope Analysis test
test_scope_analysis: $(TEST_SCOPE)
	$(TEST_SCOPE)

$(TEST_SCOPE): tests/semantic/test_scope_analysis.c $(BUILDDIR)/semantic.o $(BUILDDIR)/symbol_table.o $(BUILDDIR)/type_system.o $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Type System test
test_type_system: $(TEST_TYPE)
	$(TEST_TYPE)

$(TEST_TYPE): tests/semantic/test_type_system.c $(BUILDDIR)/type_system.o $(BUILDDIR)/token.o $(BUILDDIR)/ast.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Semantic Analyzer test
test_semantic_analyzer: $(TEST_SEM)
	$(TEST_SEM)

$(TEST_SEM): tests/semantic/test_semantic_analyzer.c $(BUILDDIR)/semantic.o $(BUILDDIR)/symbol_table.o $(BUILDDIR)/type_system.o $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run Full Semantic Integration test
test_full_semantic: $(TEST_FULL)
	$(TEST_FULL)

$(TEST_FULL): tests/semantic/test_full_semantic.c $(BUILDDIR)/semantic.o $(BUILDDIR)/symbol_table.o $(BUILDDIR)/type_system.o $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run I/O Semantic test
test_io_semantics: $(TEST_IO)
	$(TEST_IO)

$(TEST_IO): tests/semantic/test_io_semantics.c $(BUILDDIR)/semantic.o $(BUILDDIR)/symbol_table.o $(BUILDDIR)/type_system.o $(BUILDDIR)/parser.o $(BUILDDIR)/ast.o $(BUILDDIR)/lexer.o $(BUILDDIR)/token.o
	@if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

# Clean target
clean:
	-cmd /c "if exist $(BUILDDIR)\*.o del /q /f $(BUILDDIR)\*.o"
	-cmd /c "if exist $(BUILDDIR)\*.exe del /q /f $(BUILDDIR)\*.exe"
	-cmd /c "if exist $(TARGET) del /q /f $(TARGET)"
	@echo Clean complete.

.PHONY: all clean test_ast test_expressions test_statements test_program test_symbol_table test_scope_analysis test_type_system test_semantic_analyzer test_full_semantic test_io_semantics

