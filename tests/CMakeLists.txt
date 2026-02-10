# Use local Catch2 (disable warnings for third-party code)
set(CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/catch2 ${CMAKE_BINARY_DIR}/catch2)

# Restore original warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")

# Test executable
add_executable(cchess_tests
    TestMain.cpp
    core/PieceTest.cpp
    core/SquareTest.cpp
    core/PositionTest.cpp
    core/BoardTest.cpp
    core/MoveTest.cpp
    core/MoveGeneratorTest.cpp
    core/BoardMoveTest.cpp
    core/BitboardTest.cpp
    fen/FenParserTest.cpp
    fen/FenValidatorTest.cpp
    mode/PerftTest.cpp
)

target_link_libraries(cchess_tests PRIVATE
    cchess_core
    Catch2::Catch2WithMain
)

# Add tests to CTest
include(CTest)
add_test(NAME cchess_tests COMMAND cchess_tests)
set_tests_properties(cchess_tests PROPERTIES
    ENVIRONMENT "PATH=C:/msys64/ucrt64/bin;$ENV{PATH}"
)
