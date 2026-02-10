#include "core/Board.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("Board add pieces", "[board]") {
    Board board;
    board.clear();  // Start with empty board

    SECTION("Add single piece") {
        Piece whiteKing(PieceType::King, Color::White);
        board.addPiece(whiteKing, "e1");

        REQUIRE(board.at("e1").type() == PieceType::King);
        REQUIRE(board.at("e1").color() == Color::White);
    }

    SECTION("Add multiple pieces") {
        Piece whiteKing(PieceType::King, Color::White);
        Piece blackKing(PieceType::King, Color::Black);
        Piece whiteRook(PieceType::Rook, Color::White);

        board.addPiece(whiteKing, "e1");
        board.addPiece(blackKing, "e8");
        board.addPiece(whiteRook, "a1");

        REQUIRE(board.at("e1").type() == PieceType::King);
        REQUIRE(board.at("e1").color() == Color::White);
        REQUIRE(board.at("e8").type() == PieceType::King);
        REQUIRE(board.at("e8").color() == Color::Black);
        REQUIRE(board.at("a1").type() == PieceType::Rook);
        REQUIRE(board.at("a1").color() == Color::White);
    }
}

TEST_CASE("Board clear", "[board]") {
    Board board;  // Starts with standard position

    SECTION("Clear removes all pieces") {
        // Verify board has pieces initially
        REQUIRE(board.at("e1").type() == PieceType::King);
        REQUIRE(board.at("a1").type() == PieceType::Rook);

        // Clear the board
        board.clear();

        // Verify all squares are empty
        for (int rank = RANK_1; rank <= RANK_8; ++rank) {
            for (int file = FILE_A; file <= FILE_H; ++file) {
                Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(rank));
                REQUIRE(board.at(sq).type() == PieceType::None);
            }
        }
    }

    SECTION("Can add pieces after clearing") {
        board.clear();

        Piece whiteKing(PieceType::King, Color::White);
        board.addPiece(whiteKing, "d4");

        REQUIRE(board.at("d4").type() == PieceType::King);
        REQUIRE(board.at("d4").color() == Color::White);
    }
}
