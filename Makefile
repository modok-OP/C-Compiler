# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
TARGET = compiler.exe

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

# Clean target
clean:
	-cmd /c "if exist $(BUILDDIR)\*.o del /q /f $(BUILDDIR)\*.o"
	-cmd /c "if exist $(TARGET) del /q /f $(TARGET)"
	@echo Clean complete.

.PHONY: all clean
