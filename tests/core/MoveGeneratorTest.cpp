#include "core/movegen/MoveGenerator.h"
#include "fen/FenParser.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("MoveGenerator checkmate detection", "[movegen]") {
    FenParser parser;

    SECTION("Back rank mate") {
        Position pos = parser.parse("R5k1/5ppp/8/8/8/8/8/7K b - - 0 1");

        REQUIRE(MoveGenerator::isCheckmate(pos));
        REQUIRE_FALSE(MoveGenerator::isStalemate(pos));
    }

    SECTION("Scholar's mate") {
        Position pos =
            parser.parse("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 1");

        REQUIRE(MoveGenerator::isCheckmate(pos));
    }

    SECTION("Not checkmate - can block") {
        Position pos = parser.parse("R6k/6pp/5p2/8/8/1b6/8/4K3 b - - 0 1");

        // Black is in check but can block with bishop move
        REQUIRE_FALSE(MoveGenerator::isCheckmate(pos));
        REQUIRE(MoveGenerator::isInCheck(pos, Color::Black));
    }
}

TEST_CASE("MoveGenerator stalemate detection", "[movegen]") {
    FenParser parser;

    SECTION("King in corner stalemate") {
        Position pos = parser.parse("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");

        REQUIRE(MoveGenerator::isStalemate(pos));
        REQUIRE_FALSE(MoveGenerator::isCheckmate(pos));
    }

    SECTION("Not stalemate - has moves") {
        Position pos = parser.parse("7k/8/6K1/8/8/8/8/8 b - - 0 1");

        REQUIRE_FALSE(MoveGenerator::isStalemate(pos));
    }
}

TEST_CASE("MoveGenerator 50-move rule", "[movegen]") {
    FenParser parser;

    SECTION("50 moves reached") {
        Position pos = parser.parse("4k3/8/8/8/8/8/8/4K3 w - - 100 1");

        REQUIRE(MoveGenerator::isDraw(pos));
    }

    SECTION("50 moves not reached") {
        Position pos = parser.parse("4k3/8/8/8/8/8/8/4K3 w - - 99 1");

        REQUIRE_FALSE(MoveGenerator::isDraw(pos));
    }
}
