cmake_minimum_required(VERSION 3.15)
project(CChess VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(ENABLE_CPPCHECK "Enable cppcheck analysis" OFF)
option(ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enhanced compiler warnings
if(MSVC)
    add_compile_options(/W4)
    if(ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
    # Additional MSVC warnings
    add_compile_options(
        /w14242 # conversion, possible loss of data
        /w14254 # operator with different sizes
        /w14263 # member function does not override
        /w14265 # class has virtual functions but destructor is not virtual
        /w14287 # unsigned/negative constant mismatch
        /we4289 # loop variable used outside loop
        /w14296 # expression is always true/false
        /w14311 # pointer truncation
        /w14545 # missing argument list
        /w14546 # function call missing argument list
        /w14547 # operator before comma has no effect
        /w14549 # operator before operator has no effect
        /w14555 # expression has no effect
        /w14619 # pragma warning unknown
        /w14640 # construction of local static object is not thread-safe
        /w14826 # conversion is sign-extended
        /w14905 # wide string literal cast
        /w14906 # string literal cast
        /w14928 # illegal copy-initialization
    )
else()
    # GCC/Clang warnings
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow              # warn about variable shadowing
        -Wnon-virtual-dtor    # warn about non-virtual destructors
        -Wold-style-cast      # warn about C-style casts
        -Wcast-align          # warn about alignment issues
        -Wunused              # warn about unused variables
        -Woverloaded-virtual  # warn about overload hiding
        -Wconversion          # warn about type conversions
        -Wsign-conversion     # warn about sign conversions
        -Wnull-dereference    # warn about null dereference
        -Wdouble-promotion    # warn about float to double promotion
        -Wformat=2            # warn about format string issues
    )

    if(ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()

    # GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
endif()

# Clang-Tidy integration
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# Cppcheck integration
if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXE}"
            "--enable=warning,performance,portability,style"
            "--inline-suppr"
            "--suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions"
            "--std=c++17"
            "--quiet"
            "--template=gcc"
        )
    else()
        message(WARNING "cppcheck requested but not found")
    endif()
endif()

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
