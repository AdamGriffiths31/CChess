#include "core/Piece.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("Piece FEN character conversion", "[piece]") {
    SECTION("White pieces") {
        REQUIRE(Piece(PieceType::Pawn, Color::White).toFenChar() == 'P');
        REQUIRE(Piece(PieceType::Knight, Color::White).toFenChar() == 'N');
        REQUIRE(Piece(PieceType::Bishop, Color::White).toFenChar() == 'B');
        REQUIRE(Piece(PieceType::Rook, Color::White).toFenChar() == 'R');
        REQUIRE(Piece(PieceType::Queen, Color::White).toFenChar() == 'Q');
        REQUIRE(Piece(PieceType::King, Color::White).toFenChar() == 'K');
    }

    SECTION("Black pieces") {
        REQUIRE(Piece(PieceType::Pawn, Color::Black).toFenChar() == 'p');
        REQUIRE(Piece(PieceType::Knight, Color::Black).toFenChar() == 'n');
        REQUIRE(Piece(PieceType::Bishop, Color::Black).toFenChar() == 'b');
        REQUIRE(Piece(PieceType::Rook, Color::Black).toFenChar() == 'r');
        REQUIRE(Piece(PieceType::Queen, Color::Black).toFenChar() == 'q');
        REQUIRE(Piece(PieceType::King, Color::Black).toFenChar() == 'k');
    }

    SECTION("Empty piece") {
        REQUIRE(Piece().toFenChar() == ' ');
    }
}

TEST_CASE("Piece from FEN character", "[piece]") {
    SECTION("White pieces") {
        auto pawn = Piece::fromFenChar('P');
        REQUIRE(pawn.type() == PieceType::Pawn);
        REQUIRE(pawn.color() == Color::White);

        auto king = Piece::fromFenChar('K');
        REQUIRE(king.type() == PieceType::King);
        REQUIRE(king.color() == Color::White);
    }

    SECTION("Black pieces") {
        auto pawn = Piece::fromFenChar('p');
        REQUIRE(pawn.type() == PieceType::Pawn);
        REQUIRE(pawn.color() == Color::Black);

        auto king = Piece::fromFenChar('k');
        REQUIRE(king.type() == PieceType::King);
        REQUIRE(king.color() == Color::Black);
    }

    SECTION("Invalid character") {
        auto invalid = Piece::fromFenChar('X');
        REQUIRE(invalid.isEmpty());
    }
}

TEST_CASE("Piece Unicode symbols", "[piece]") {
    SECTION("White pieces contain Unicode") {
        auto pawn = Piece(PieceType::Pawn, Color::White);
        auto unicode = pawn.toUnicode();
        REQUIRE(!unicode.empty());
        REQUIRE(unicode == "\u2659");
    }

    SECTION("Black pieces contain Unicode") {
        auto king = Piece(PieceType::King, Color::Black);
        auto unicode = king.toUnicode();
        REQUIRE(!unicode.empty());
        REQUIRE(unicode == "\u265A");
    }

    SECTION("Empty piece") {
        auto empty = Piece();
        REQUIRE(empty.toUnicode() == " ");
    }
}

TEST_CASE("Piece ASCII representation", "[piece]") {
    SECTION("White pieces") {
        REQUIRE(Piece(PieceType::Pawn, Color::White).toAscii() == 'P');
        REQUIRE(Piece(PieceType::King, Color::White).toAscii() == 'K');
    }

    SECTION("Black pieces") {
        REQUIRE(Piece(PieceType::Pawn, Color::Black).toAscii() == 'p');
        REQUIRE(Piece(PieceType::King, Color::Black).toAscii() == 'k');
    }

    SECTION("Empty piece") {
        REQUIRE(Piece().toAscii() == '.');
    }
}

TEST_CASE("Piece equality", "[piece]") {
    auto piece1 = Piece(PieceType::Pawn, Color::White);
    auto piece2 = Piece(PieceType::Pawn, Color::White);
    auto piece3 = Piece(PieceType::Pawn, Color::Black);

    REQUIRE(piece1 == piece2);
    REQUIRE(piece1 != piece3);
}
