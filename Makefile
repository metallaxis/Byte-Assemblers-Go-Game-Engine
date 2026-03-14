# ============================================================
#   C Cross-Platform Build System
# ============================================================

# Default Compiler (will be set based on platform)
CC = none

# Default Compiler Flags
CFLAGS = none

# Default Platform (will be detected)
PLATFORM = none
ifeq ($(OS),Windows_NT)
	PLATFORM = windows
else
	PLATFORM = linux
endif

# Debug flag
DEBUG = -g -DDEBUG=1

# ------------------------------------------------------------
# Include auto-generated dependency files
-include $(OBJ_FILES:.o=.d)

# ------------------------------------------------------------
# General information
INCLUDE_FLAGS = -Iinclude

# Linux-specific information
LINUX_CC = gcc
LINUX_CFLAGS = -O3 $(INCLUDE_FLAGS) -static -MMD -MP -Wall -Wextra -Werror -pedantic
LINUX_LDFLAGS = $(LINUX_CFLAGS)

# Windows-specific information
WINDOWS_CC = x86_64-w64-mingw32-gcc
WINDOWS_CFLAGS = -O3 $(INCLUDE_FLAGS) -static -MMD -MP -Wall -Wextra -Werror -pedantic
WINDOWS_LDFLAGS = $(WINDOWS_CFLAGS)

# ------------------------------------------------------------
# Project folders
SRC = src
BIN_DIR = ./bin
OBJ_DIR = ./obj

# ------------------------------------------------------------
# Detect platform based on make command arguments
ifneq (,$(findstring linux,$(MAKECMDGOALS)))
PLATFORM := linux
endif

ifneq (,$(findstring windows,$(MAKECMDGOALS)))
PLATFORM := windows
endif

# ------------------------------------------------------------
# Project source files
SRC_FILES := $(shell find $(SRC) -name '*.c')
OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
TARGET = $(BIN_DIR)/betago

# ------------------------------------------------------------
# Output binary name
ifeq ($(PLATFORM), windows)
	CC = $(WINDOWS_CC)
	CFLAGS = $(WINDOWS_CFLAGS)
	LDFLAGS = $(WINDOWS_LDFLAGS)
	OUT = $(TARGET).exe
else ifeq ($(PLATFORM), linux)
	CC = $(LINUX_CC)
	CFLAGS = $(LINUX_CFLAGS)
	LDFLAGS = $(LINUX_LDFLAGS)
	OUT = $(TARGET)
endif

# ------------------------------------------------------------
# Default Platform (will be detected)
default: $(OUT)

# ------------------------------------------------------------
# Help target: Display usage help
help:
	@echo "Usage:"
	@echo "  make                               - Default Build for your operating system"
	@echo "  make build linux|windows           - Build for your specified platform"
	@echo "  make debug linux|windows           - Build with debug symbols for the engine"
	@echo "  make run linux|windows             - Run the built program"
	@echo "  make build --debug linux|windows   - Build with debug symbols for make"
	@echo "  make debug --debug linux|windows   - Build with debug symbols for make and the engine"
	@echo "  make run --debug linux|windows     - Run with debug symbols"
	@echo "  make clean                         - Clean the build directory"

# ------------------------------------------------------------
# Ensure build directories exist
check_dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# ------------------------------------------------------------
$(OBJ_DIR)/%.o: %.c | check_dirs
	@mkdir -p $(dir $@)
	@echo "Compiling $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------
$(OUT): $(OBJ_FILES) | check_dirs
	@echo "Linking final executable: $(OUT)"
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(OUT) $(LDFLAGS)
	ln -s ../goref/goref.py $(BIN_DIR)/goref

# ------------------------------------------------------------
# Build target
build: $(OUT)

# ------------------------------------------------------------
# Run target
run: build
ifeq ($(PLATFORM), windows)
	@echo "Running application..."
	@start "" "$(OUT)"
else ifeq ($(PLATFORM), linux)
	@echo "Running application..."
	$(OUT)
else
	@echo "Specify a platform: make run linux or make run windows"
endif

# ------------------------------------------------------------
# Clean target
clean:
	@echo "Cleaning object and bin directories..."
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# ------------------------------------------------------------
# Declare linux/windows as fake targets so make doesn’t fail
.PHONY: linux windows
linux:
	@true
windows:
	@true

# Debug flag
.PHONY: debug
debug: CFLAGS += DEBUG
debug: $(OUT)
