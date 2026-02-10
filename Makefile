# Makefile for CChess project

# Configuration
BUILD_DIR_DEBUG := build/debug
BUILD_DIR_RELEASE := build/release
BUILD_DIR := $(BUILD_DIR_RELEASE)
BIN_DIR := $(BUILD_DIR)/bin
EXECUTABLE := $(BIN_DIR)/cchess.exe
CMAKE_GENERATOR := -G "MinGW Makefiles"
# MSYS2 ucrt64 toolchain
UCRT64 := /c/msys64/ucrt64/bin
CMAKE_FLAGS_COMMON := -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_CXX_COMPILER=$(UCRT64)/g++.exe

# Fix MSYS2 TMP path for native Windows compilers (g++ can't use POSIX /tmp)
export TMP := $(shell cygpath -w /tmp)
export TEMP := $(TMP)

# Runtime PATH for ucrt64 DLLs (libstdc++, libgcc)
RUNTIME_PATH := PATH="$(UCRT64):$$PATH"

# Default target (release build)
.PHONY: all
all: release

# Configure CMake for Release
.PHONY: configure-release
configure-release:
	@cmake $(CMAKE_GENERATOR) -B $(BUILD_DIR_RELEASE) $(CMAKE_FLAGS_COMMON) -DCMAKE_BUILD_TYPE=Release

# Configure CMake for Debug
.PHONY: configure-debug
configure-debug:
	@cmake $(CMAKE_GENERATOR) -B $(BUILD_DIR_DEBUG) $(CMAKE_FLAGS_COMMON) -DCMAKE_BUILD_TYPE=Debug

# Build Release (default)
.PHONY: build release
build: release
release: configure-release
	@cmake --build $(BUILD_DIR_RELEASE) --parallel 4

# Build Debug (with assertions enabled)
.PHONY: debug
debug: configure-debug
	@cmake --build $(BUILD_DIR_DEBUG) --parallel 4

# Run the executable (release)
.PHONY: run
run: release
	@$(RUNTIME_PATH) $(BUILD_DIR_RELEASE)/bin/cchess.exe

# Run debug executable
.PHONY: run-debug
run-debug: debug
	@$(RUNTIME_PATH) $(BUILD_DIR_DEBUG)/bin/cchess.exe

# Clean build artifacts
.PHONY: clean
clean:
	@rm -rf build

# Clean only debug build
.PHONY: clean-debug
clean-debug:
	@rm -rf $(BUILD_DIR_DEBUG)

# Clean only release build
.PHONY: clean-release
clean-release:
	@rm -rf $(BUILD_DIR_RELEASE)

# Build and run tests (DEBUG - with assertions)
.PHONY: test
test: debug
	@$(RUNTIME_PATH) $(BUILD_DIR_DEBUG)/bin/cchess_tests.exe

# Build and run tests with detailed output (DEBUG)
.PHONY: test-verbose
test-verbose: debug
	@$(RUNTIME_PATH) $(BUILD_DIR_DEBUG)/bin/cchess_tests.exe --reporter console --success

# Build and run tests in Release mode (no assertions)
.PHONY: test-release
test-release: release
	@$(RUNTIME_PATH) $(BUILD_DIR_RELEASE)/bin/cchess_tests.exe

# Run benchmarks (release build for meaningful numbers)
.PHONY: bench
bench: release
	@$(RUNTIME_PATH) $(BUILD_DIR_RELEASE)/bin/cchess_tests.exe [bench]

# Format code with clang-format
.PHONY: format
format:
	@echo "Formatting C++ code..."
	@find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Formatting complete!"

# Run static analysis checks
.PHONY: check
check:
	@echo "Running static analysis with cppcheck..."
	@cppcheck --enable=warning,performance,portability,style --std=c++17 \
		--suppressions-list=.cppcheck-suppressions --inline-suppr --quiet \
		--template=gcc src/ 2>&1 | grep -v "Checking" | grep -v "^$$" || true
	@echo "All checks complete!"

# Format check (verify formatting without changes)
.PHONY: format-check
format-check:
	@echo "Checking code formatting..."
	@find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
	@echo "All files properly formatted!"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all            - Build release version (default)"
	@echo "  release        - Build optimized release (assertions OFF)"
	@echo "  debug          - Build debug version (assertions ON)"
	@echo "  run            - Build and run release executable"
	@echo "  run-debug      - Build and run debug executable"
	@echo ""
	@echo "Test targets:"
	@echo "  test           - Build and run tests in DEBUG mode"
	@echo "  test-verbose   - Run tests with detailed output (DEBUG)"
	@echo "  test-release   - Run tests in RELEASE mode (no assertions)"
	@echo "  bench          - Run benchmarks (release build)"
	@echo ""
	@echo "Development targets:"
	@echo "  format         - Format code with clang-format"
	@echo "  format-check   - Check if code is properly formatted"
	@echo "  check          - Run static analysis (cppcheck)"
	@echo ""
	@echo "Clean targets:"
	@echo "  clean          - Remove all build artifacts"
	@echo "  clean-debug    - Remove debug build only"
	@echo "  clean-release  - Remove release build only"
	@echo ""
	@echo "Other:"
	@echo "  help           - Show this help message"
