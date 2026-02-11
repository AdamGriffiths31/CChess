#include "core/Board.h"
#include "core/movegen/MoveGenerator.h"
#include "fen/FenParser.h"
#include "mode/PerftRunner.h"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace cchess;

static uint64_t perft(Board& board, int depth) {
    if (depth == 0)
        return 1;

    Position& pos = board.position();
    MoveList pseudo = MoveGenerator::generatePseudoLegalMoves(pos);
    Color us = pos.sideToMove();

    uint64_t nodes = 0;
    for (const auto& move : pseudo) {
        UndoInfo undo = pos.makeMove(move);
        if (!MoveGenerator::isInCheck(pos, us)) {
            nodes += (depth == 1) ? 1 : perft(board, depth - 1);
        }
        pos.unmakeMove(move, undo);
    }
    return nodes;
}

// Detailed perft result with move type breakdown
struct PerftResult {
    uint64_t nodes = 0;
    uint64_t captures = 0;
    uint64_t enPassant = 0;
    uint64_t castles = 0;
    uint64_t promotions = 0;
    uint64_t checks = 0;
    uint64_t checkmates = 0;

    PerftResult& operator+=(const PerftResult& other) {
        nodes += other.nodes;
        captures += other.captures;
        enPassant += other.enPassant;
        castles += other.castles;
        promotions += other.promotions;
        checks += other.checks;
        checkmates += other.checkmates;
        return *this;
    }
};

static PerftResult perftDetailed(Board& board, int depth) {
    PerftResult result;
    if (depth == 0) {
        result.nodes = 1;
        return result;
    }

    Position& pos = board.position();
    MoveList pseudo = MoveGenerator::generatePseudoLegalMoves(pos);
    Color us = pos.sideToMove();

    for (const auto& move : pseudo) {
        UndoInfo undo = pos.makeMove(move);

        if (MoveGenerator::isInCheck(pos, us)) {
            pos.unmakeMove(move, undo);
            continue;
        }

        if (depth == 1) {
            result.nodes++;
            if (move.isCapture())
                result.captures++;
            if (move.isEnPassant())
                result.enPassant++;
            if (move.isCastling())
                result.castles++;
            if (move.isPromotion())
                result.promotions++;
            if (board.isInCheck())
                result.checks++;
            if (board.isCheckmate())
                result.checkmates++;
        } else {
            result += perftDetailed(board, depth - 1);
        }

        pos.unmakeMove(move, undo);
    }
    return result;
}

// Initial position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// Reference: https://www.chessprogramming.org/Perft_Results
TEST_CASE("Perft - Initial Position", "[perft]") {
    Board board;

    SECTION("Depth 1") {
        auto result = perftDetailed(board, 1);
        REQUIRE(result.nodes == 20);
        REQUIRE(result.captures == 0);
        REQUIRE(result.enPassant == 0);
        REQUIRE(result.castles == 0);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 0);
    }

    SECTION("Depth 2") {
        auto result = perftDetailed(board, 2);
        REQUIRE(result.nodes == 400);
        REQUIRE(result.captures == 0);
        REQUIRE(result.enPassant == 0);
        REQUIRE(result.castles == 0);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 0);
    }

    SECTION("Depth 3") {
        auto result = perftDetailed(board, 3);
        REQUIRE(result.nodes == 8'902);
        REQUIRE(result.captures == 34);
        REQUIRE(result.enPassant == 0);
        REQUIRE(result.castles == 0);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 12);
    }

    SECTION("Depth 4") {
        auto result = perftDetailed(board, 4);
        REQUIRE(result.nodes == 197'281);
        REQUIRE(result.captures == 1576);
        REQUIRE(result.enPassant == 0);
        REQUIRE(result.castles == 0);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 469);
    }

    SECTION("Depth 5") {
        auto result = perftDetailed(board, 5);
        REQUIRE(result.nodes == 4'865'609);
        REQUIRE(result.captures == 82'719);
        REQUIRE(result.enPassant == 258);
        REQUIRE(result.castles == 0);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 27'351);
    }
}

// Kiwipete: r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
// Reference: https://www.chessprogramming.org/Perft_Results#Position_2
TEST_CASE("Perft - Kiwipete", "[perft]") {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    SECTION("Depth 1") {
        auto result = perftDetailed(board, 1);
        REQUIRE(result.nodes == 48);
        REQUIRE(result.captures == 8);
        REQUIRE(result.enPassant == 0);
        REQUIRE(result.castles == 2);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 0);
    }

    SECTION("Depth 2") {
        auto result = perftDetailed(board, 2);
        REQUIRE(result.nodes == 2'039);
        REQUIRE(result.captures == 351);
        REQUIRE(result.enPassant == 1);
        REQUIRE(result.castles == 91);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 3);
    }

    SECTION("Depth 3") {
        auto result = perftDetailed(board, 3);
        REQUIRE(result.nodes == 97'862);
        REQUIRE(result.captures == 17'102);
        REQUIRE(result.enPassant == 45);
        REQUIRE(result.castles == 3'162);
        REQUIRE(result.promotions == 0);
        REQUIRE(result.checks == 993);
    }

    SECTION("Depth 4") {
        auto result = perftDetailed(board, 4);
        REQUIRE(result.nodes == 4'085'603);
        REQUIRE(result.captures == 757'163);
        REQUIRE(result.enPassant == 1'929);
        REQUIRE(result.castles == 128'013);
        REQUIRE(result.promotions == 15'172);
        REQUIRE(result.checks == 25'523);
    }

    SECTION("Depth 5") {
        auto result = perftDetailed(board, 5);
        REQUIRE(result.nodes == 193'690'690);
        REQUIRE(result.captures == 35'043'416);
        REQUIRE(result.enPassant == 73'365);
        REQUIRE(result.castles == 4'993'637);
        REQUIRE(result.promotions == 8'392);
        REQUIRE(result.checks == 3'309'887);
    }
}

// Problematic: rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8
// Reference: https://www.chessprogramming.org/Perft_Results#Position_5
TEST_CASE("Perft - Problematic", "[perft]") {
    Board board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");

    SECTION("Depth 1") {
        auto result = perftDetailed(board, 1);
        REQUIRE(result.nodes == 44);
    }

    SECTION("Depth 2") {
        auto result = perftDetailed(board, 2);
        REQUIRE(result.nodes == 1'486);
    }

    SECTION("Depth 3") {
        auto result = perftDetailed(board, 3);
        REQUIRE(result.nodes == 62'379);
    }
}

// ============================================================================
// Benchmarks
// ============================================================================

TEST_CASE("Bench - Kiwipete depth 4", "[bench]") {
    Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    auto start = std::chrono::steady_clock::now();
    uint64_t nodes = perft(board, 4);
    auto end = std::chrono::steady_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double nps = static_cast<double>(nodes) / (ms / 1000.0);

    REQUIRE(nodes == 4'085'603);

    std::cout << "\n=== Kiwipete Depth 4 ===\n"
              << "  Nodes: " << nodes << "\n"
              << "  Time:  " << static_cast<int>(ms) << " ms\n"
              << "  NPS:   " << static_cast<int>(nps) << "\n";
}