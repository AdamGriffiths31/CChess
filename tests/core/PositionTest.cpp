#include "core/Position.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Helper constants for tests
constexpr Square SQUARE_E4 = makeSquare(FILE_E, RANK_4);
constexpr Square SQUARE_E3 = makeSquare(FILE_E, RANK_3);

TEST_CASE("Position initialization", "[position]") {
    Position pos;

    SECTION("Default state") {
        REQUIRE(pos.sideToMove() == Color::White);
        REQUIRE(pos.castlingRights() == NO_CASTLING);
        REQUIRE(pos.enPassantSquare() == SQUARE_NONE);
        REQUIRE(pos.halfmoveClock() == 0);
        REQUIRE(pos.fullmoveNumber() == 1);
    }

    SECTION("Board is empty") {
        for (Square sq = 0; sq < 64; ++sq) {
            REQUIRE(pos.pieceAt(sq).isEmpty());
        }
    }
}

TEST_CASE("Position piece placement", "[position]") {
    Position pos;

    SECTION("Set and get piece") {
        Piece piece(PieceType::Pawn, Color::White);
        pos.setPiece(SQUARE_E4, piece);
        REQUIRE(pos.pieceAt(SQUARE_E4) == piece);
    }

    SECTION("Clear square") {
        Piece piece(PieceType::Pawn, Color::White);
        pos.setPiece(SQUARE_E4, piece);
        pos.clearSquare(SQUARE_E4);
        REQUIRE(pos.pieceAt(SQUARE_E4).isEmpty());
    }

    SECTION("Clear board") {
        pos.setPiece(SQUARE_E4, Piece(PieceType::Pawn, Color::White));
        pos.clear();
        REQUIRE(pos.pieceAt(SQUARE_E4).isEmpty());
    }
}

TEST_CASE("Position game state", "[position]") {
    Position pos;

    SECTION("Side to move") {
        pos.setSideToMove(Color::Black);
        REQUIRE(pos.sideToMove() == Color::Black);
    }

    SECTION("Castling rights") {
        pos.setCastlingRights(WHITE_KINGSIDE | BLACK_QUEENSIDE);
        REQUIRE(pos.castlingRights() == (WHITE_KINGSIDE | BLACK_QUEENSIDE));
    }

    SECTION("En passant square") {
        pos.setEnPassantSquare(SQUARE_E3);
        REQUIRE(pos.enPassantSquare() == SQUARE_E3);
    }

    SECTION("Halfmove clock") {
        pos.setHalfmoveClock(42);
        REQUIRE(pos.halfmoveClock() == 42);
    }

    SECTION("Fullmove number") {
        pos.setFullmoveNumber(10);
        REQUIRE(pos.fullmoveNumber() == 10);
    }
}
