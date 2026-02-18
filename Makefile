# CChess Makefile

UCRT64 := /c/msys64/ucrt64/bin
PATH_FIX := PATH="$(UCRT64):$$PATH"
CMAKE_MINGW := -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=$(UCRT64)/g++.exe
CMAKE_MSVC := -G "Visual Studio 18 2026" -A x64 -DFETCHCONTENT_FULLY_DISCONNECTED=ON

# Fix MSYS2 TMP path for native Windows compilers
export TMP := $(shell cygpath -w /tmp)
export TEMP := $(TMP)

.PHONY: all release debug msvc run run-debug run-msvc test test-verbose test-release bench format format-check check clean help

all: release

release:
	@cmake $(CMAKE_MINGW) -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --parallel 4

debug:
	@cmake $(CMAKE_MINGW) -B build/debug -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build/debug --parallel 4

msvc:
	@cmake $(CMAKE_MSVC) -B build/msvc
	@cmake --build build/msvc --config RelWithDebInfo --parallel 4

run: release
	@$(PATH_FIX) build/release/bin/cchess.exe

run-debug: debug
	@$(PATH_FIX) build/debug/bin/cchess.exe

run-msvc: msvc
	@build/msvc/bin/RelWithDebInfo/cchess.exe

test: debug
	@$(PATH_FIX) build/debug/bin/cchess_tests.exe

test-verbose: debug
	@$(PATH_FIX) build/debug/bin/cchess_tests.exe --reporter console --success

test-release: release
	@$(PATH_FIX) build/release/bin/cchess_tests.exe

bench: release
	@$(PATH_FIX) build/release/bin/cchess_tests.exe [bench]

format:
	@find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i

format-check:
	@find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror

check:
	@cppcheck --enable=warning,performance,portability,style --std=c++17 \
		--suppressions-list=.cppcheck-suppressions --inline-suppr --quiet \
		--template=gcc src/ 2>&1 | grep -v "Checking" | grep -v "^$$" || true

clean:
	@rm -rf build

help:
	@echo "make release     Build optimized (default)"
	@echo "make debug       Build with assertions"
	@echo "make msvc        Build MSVC RelWithDebInfo (for VS profiler)"
	@echo "make run         Build + run release"
	@echo "make run-debug   Build + run debug"
	@echo "make test        Build + run tests (debug)"
	@echo "make test-release  Run tests (release)"
	@echo "make bench       Run benchmarks (release)"
	@echo "make format      Format code with clang-format"
	@echo "make check       Run cppcheck"
	@echo "make clean       Remove all build artifacts"
