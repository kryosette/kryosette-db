# ============================================================================
# Arena Allocator Build System
# ============================================================================

# Project Configuration
PROJECT_NAME = arena_allocator
TARGET = arena_example

# Directories
SRC_DIR = .
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
INCLUDE_DIR = include

# Source Files
CONSTANTS_SRC = $(SRC_DIR)/constants.c
EXAMPLE_SRC = $(SRC_DIR)/example.c
CORE_SRC = $(SRC_DIR)/core.c

# Header Files
CONSTANTS_H = $(INCLUDE_DIR)/constants.h
CORE_H = $(INCLUDE_DIR)/core.h

# Object Files
CONSTANTS_OBJ = $(OBJ_DIR)/constants.o
EXAMPLE_OBJ = $(OBJ_DIR)/example.o
CORE_OBJ = $(OBJ_DIR)/core.o

# All Object Files
OBJS = $(CONSTANTS_OBJ) $(EXAMPLE_OBJ) $(CORE_OBJ)

# Compiler and Flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2 -g
CFLAGS += -I$(INCLUDE_DIR) -I$(SRC_DIR)
LDFLAGS = -pthread
RM = rm -f
MKDIR = mkdir -p

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM := del /Q
    MKDIR := mkdir
endif

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean directories

# Default target
all: directories $(BIN_DIR)/$(TARGET)

# Create necessary directories
directories:
	@echo "Creating build directories..."
	$(MKDIR) $(BUILD_DIR)
	$(MKDIR) $(OBJ_DIR)
	$(MKDIR) $(BIN_DIR)

# Link final executable
$(BIN_DIR)/$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build completed: $@"

# Compile constants.c
$(CONSTANTS_OBJ): $(CONSTANTS_SRC) $(CONSTANTS_H)
	@echo "Compiling constants.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile example.c
$(EXAMPLE_OBJ): $(EXAMPLE_SRC) $(CORE_H) $(CONSTANTS_H)
	@echo "Compiling example.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile core.c
$(CORE_OBJ): $(CORE_SRC) $(CORE_H) $(CONSTANTS_H)
	@echo "Compiling core.c..."
	$(CC) $(CFLAGS) -c $< -o $@

# Header dependencies
$(CONSTANTS_OBJ): $(CONSTANTS_H)
$(EXAMPLE_OBJ): $(CORE_H) $(CONSTANTS_H)
$(CORE_OBJ): $(CORE_H) $(CONSTANTS_H)

# ============================================================================
# Development and Debug Rules
# ============================================================================

# Debug build with extra warnings
debug: CFLAGS += -DDEBUG -O0 -fsanitize=address -fsanitize=undefined
debug: clean all

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG
release: clean all

# Run the example program
run: all
	@echo "Running $(TARGET)..."
	$(BIN_DIR)/$(TARGET)

# Valgrind memory check
valgrind: all
	@echo "Running valgrind memory check..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BIN_DIR)/$(TARGET)

# Run with gdb debugger
gdb: debug
	@echo "Starting gdb debugger..."
	gdb $(BIN_DIR)/$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(RM) -r $(BUILD_DIR)
	@echo "Clean completed."

# Install (optional - for system-wide installation)
install: all
	@echo "Installing $(TARGET) to /usr/local/bin..."
	cp $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	chmod +x /usr/local/bin/$(TARGET)

# Uninstall
uninstall:
	@echo "Uninstalling $(TARGET)..."
	$(RM) /usr/local/bin/$(TARGET)

# ============================================================================
# Testing Rules
# ============================================================================

# Run simple test
test: all
	@echo "Running basic test..."
	@if $(BIN_DIR)/$(TARGET); then \
		echo "Test passed!"; \
	else \
		echo "Test failed!"; \
		exit 1; \
	fi

# Generate assembly output
asm: $(EXAMPLE_SRC)
	@echo "Generating assembly..."
	$(CC) $(CFLAGS) -S $< -o $(BUILD_DIR)/example.s
	$(CC) $(CFLAGS) -S $(CORE_SRC) -o $(BUILD_DIR)/core.s
	@echo "Assembly files generated in $(BUILD_DIR)/"

# Show file sizes
size: all
	@echo "File sizes:"
	@size $(BIN_DIR)/$(TARGET) $(OBJS)

# Show dependencies
deps:
	@echo "Source dependencies:"
	@echo "constants.c depends on:" $(shell $(CC) -MM $(CONSTANTS_SRC))
	@echo "example.c depends on:" $(shell $(CC) -MM $(EXAMPLE_SRC))
	@echo "core.c depends on:" $(shell $(CC) -MM $(CORE_SRC))

# ============================================================================
# Helper Rules
# ============================================================================

# List all available targets
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove all build artifacts"
	@echo "  debug     - Build with debug flags and sanitizers"
	@echo "  release   - Build with optimizations"
	@echo "  run       - Build and run the example"
	@echo "  test      - Build and run tests"
	@echo "  valgrind  - Run with valgrind memory checker"
	@echo "  gdb       - Run with gdb debugger"
	@echo "  asm       - Generate assembly code"
	@echo "  size      - Show binary and object sizes"
	@echo "  deps      - Show file dependencies"
	@echo "  help      - Show this help message"

# Show build configuration
config:
	@echo "Build Configuration:"
	@echo "  Project: $(PROJECT_NAME)"
	@echo "  Target: $(TARGET)"
	@echo "  Compiler: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  Source files:"
	@echo "    - $(CONSTANTS_SRC)"
	@echo "    - $(EXAMPLE_SRC)"
	@echo "    - $(CORE_SRC)"
	@echo "  Header files:"
	@echo "    - $(CONSTANTS_H)"
	@echo "    - $(CORE_H)"

# ============================================================================
# Dependency generation (optional)
# ============================================================================

# Generate dependency files
DEP_FILES = $(OBJS:.o=.d)

-include $(DEP_FILES)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@$(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@

# ============================================================================
# File Structure
# ============================================================================

# Expected file structure:
# .
# ├── Makefile
# ├── constants.c
# ├── example.c
# ├── core.c
# ├── include/
# │   ├── constants.h
# │   └── core.h
# └── build/
#     ├── obj/
#     │   ├── constants.o
#     │   ├── example.o
#     │   └── core.o
#     └── bin/
#         └── arena_example