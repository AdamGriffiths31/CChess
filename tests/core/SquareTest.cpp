#include "core/Square.h"
#include "core/Types.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("Square to string conversion", "[square]") {
    REQUIRE(squareToString(SQUARE_A1) == "a1");
    REQUIRE(squareToString(SQUARE_H1) == "h1");
    REQUIRE(squareToString(SQUARE_A8) == "a8");
    REQUIRE(squareToString(SQUARE_H8) == "h8");
    REQUIRE(squareToString(makeSquare(FILE_E, RANK_4)) == "e4");
}

TEST_CASE("String to square conversion", "[square]") {
    REQUIRE(stringToSquare("a1") == SQUARE_A1);
    REQUIRE(stringToSquare("h1") == SQUARE_H1);
    REQUIRE(stringToSquare("a8") == SQUARE_A8);
    REQUIRE(stringToSquare("h8") == SQUARE_H8);
    REQUIRE(stringToSquare("e4") == makeSquare(FILE_E, RANK_4));

    SECTION("Case insensitive") {
        REQUIRE(stringToSquare("E4") == makeSquare(FILE_E, RANK_4));
    }

    SECTION("Invalid strings") {
        REQUIRE(!stringToSquare(""));
        REQUIRE(!stringToSquare("a"));
        REQUIRE(!stringToSquare("a9"));
        REQUIRE(!stringToSquare("i1"));
        REQUIRE(!stringToSquare("abc"));
    }
}

TEST_CASE("File and rank extraction", "[square]") {
    Square sq = makeSquare(FILE_E, RANK_4);
    REQUIRE(getFile(sq) == FILE_E);
    REQUIRE(getRank(sq) == RANK_4);
}

TEST_CASE("File/Rank character conversion", "[square]") {
    REQUIRE(fileToChar(FILE_A) == 'a');
    REQUIRE(fileToChar(FILE_H) == 'h');
    REQUIRE(rankToChar(RANK_1) == '1');
    REQUIRE(rankToChar(RANK_8) == '8');

    REQUIRE(charToFile('a') == FILE_A);
    REQUIRE(charToFile('h') == FILE_H);
    REQUIRE(charToFile('A') == FILE_A);
    REQUIRE(charToRank('1') == RANK_1);
    REQUIRE(charToRank('8') == RANK_8);
}

TEST_CASE("Square validity", "[square]") {
    REQUIRE(squareIsValid(0));
    REQUIRE(squareIsValid(63));
    REQUIRE(!squareIsValid(64));
    REQUIRE(!squareIsValid(SQUARE_NONE));
}
