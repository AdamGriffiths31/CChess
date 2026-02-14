# Core library
add_library(cchess_core STATIC
    # Core
    core/Piece.cpp
    core/Square.cpp
    core/Position.cpp
    core/Bitboard.cpp
    core/Board.cpp
    core/Move.cpp
    core/Notation.cpp
    core/Zobrist.cpp
    core/movegen/MoveGenerator.cpp
    core/movegen/AttackTables.cpp

    # FEN
    fen/FenParser.cpp
    fen/FenValidator.cpp

    # Display
    display/BoardRenderer.cpp

    # AI
    ai/Eval.cpp
    ai/MoveOrder.cpp
    ai/Search.cpp
    ai/TranspositionTable.cpp

    # Mode
    mode/PlayerVsPlayer.cpp
    mode/PerftRunner.cpp
    mode/StsRunner.cpp
    mode/OpponentList.cpp
    mode/EngineMatch.cpp

    # UCI
    uci/Uci.cpp
    uci/UciEngine.cpp

    # Utils
    utils/StringUtils.cpp
)

target_include_directories(cchess_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cchess_core PUBLIC nlohmann_json::nlohmann_json)

# Main executable
add_executable(cchess main.cpp)
target_link_libraries(cchess PRIVATE cchess_core)

# Platform-specific libraries
if(WIN32)
    # No additional libraries needed on Windows
else()
    # No additional libraries needed on Unix
endif()
