# CChess Makefile

# Detect OS
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    UCRT64      := /c/msys64/ucrt64/bin
    PATH_FIX    := PATH="$(UCRT64):$$PATH"
    CMAKE_MINGW := -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=$(UCRT64)/g++.exe
    CMAKE_MSVC  := -G "Visual Studio 18 2026" -A x64 -DFETCHCONTENT_FULLY_DISCONNECTED=ON
    EXE         := .exe
    # Fix MSYS2 TMP path for native Windows compilers
    export TMP  := $(shell cygpath -w /tmp)
    export TEMP := $(TMP)
else
    DETECTED_OS := Linux
    PATH_FIX    :=
    CMAKE_LINUX := -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    EXE         :=
endif

.PHONY: all release debug msvc run run-debug run-msvc test test-verbose test-release bench profile-bench format format-check check clean help

all: release

ifeq ($(DETECTED_OS),Windows)

release:
	@cmake $(CMAKE_MINGW) -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --parallel 4

debug:
	@cmake $(CMAKE_MINGW) -B build/debug -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build/debug --parallel 4

msvc:
	@cmake $(CMAKE_MSVC) -B build/msvc
	@cmake --build build/msvc --config RelWithDebInfo --parallel 4

run-msvc: msvc
	@build/msvc/bin/RelWithDebInfo/cchess.exe

else

release:
	@cmake $(CMAKE_LINUX) -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --parallel 4

debug:
	@cmake $(CMAKE_LINUX) -B build/debug -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build/debug --parallel 4

msvc:
	@echo "MSVC target is only available on Windows"
	@exit 1

run-msvc:
	@echo "run-msvc target is only available on Windows"
	@exit 1

endif

run: release
	@$(PATH_FIX) build/release/bin/cchess$(EXE)

run-debug: debug
	@$(PATH_FIX) build/debug/bin/cchess$(EXE)

test: debug
	@$(PATH_FIX) build/debug/bin/cchess_tests$(EXE)

test-verbose: debug
	@$(PATH_FIX) build/debug/bin/cchess_tests$(EXE) --reporter console --success

test-release: release
	@$(PATH_FIX) build/release/bin/cchess_tests$(EXE)

bench: release
	@$(PATH_FIX) build/release/bin/cchess_tests$(EXE) [bench]

profile-bench:
	@cmake $(CMAKE_LINUX) -B build/profile -DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cmake --build build/profile --parallel 4
	@LD_PRELOAD=/usr/lib/libprofiler.so CPUPROFILE=/tmp/cchess.prof CPUPROFILE_FREQUENCY=1000 \
		build/profile/bin/cchess$(EXE) --bench $(TIME)
	@echo ""
	@echo "Profile saved to /tmp/cchess.prof"
	@echo "Analyze with: pprof -text build/profile/bin/cchess /tmp/cchess.prof"
	@echo "Or web UI:    pprof -http=:8080 build/profile/bin/cchess /tmp/cchess.prof"

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
	@echo "make msvc        Build MSVC RelWithDebInfo (Windows only)"
	@echo "make run         Build + run release"
	@echo "make run-debug   Build + run debug"
	@echo "make test        Build + run tests (debug)"
	@echo "make test-release  Run tests (release)"
	@echo "make bench       Run benchmarks (release)"
	@echo "make profile-bench [TIME=ms]  Profile search with gperftools (default 30s)"
	@echo "make format      Format code with clang-format"
	@echo "make check       Run cppcheck"
	@echo "make clean       Remove all build artifacts"
