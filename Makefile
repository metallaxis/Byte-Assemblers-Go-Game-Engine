## Short makefile to compile my Go engine

## You SHOULD NOT modify the parameters below

## Compiler to use - do not change
CC = gcc

## Compiler flags - do not change
CFLAGS = -static -Wall -Wextra -Werror -pedantic -O3

## Linking flags - do not change
LDFLAGS = $(CFLAGS)

## The name of the binary (executable) file - do not change
TARGET ?= goteam

## Build the target by default
all: $(TARGET)

## You can change everything below to make your target compile,
## link, clean or do anything else you'd like.

# Include the header files
CFLAGS += -Iinclude

# Project folders
SRC = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
OBJ_DIR = ./obj

 # Project source files
SRC_FILES = $(shell find $(SRC) -name '*.c')
OBJ_FILES = $(patsubst $(SRC)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC)/%.c $(INCLUDE_DIR)/%.h | $(OBJ_DIR)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	@echo "Linking final executable: $(TARGET)"
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)

# Clean target
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

# Debug flag
.PHONY: debug
debug: CFLAGS += -g
debug: $(TARGET)
