#include "fen/FenParser.h"
#include "fen/FenValidator.h"
#include "utils/Error.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Helper constants for tests
constexpr Square SQUARE_E1 = makeSquare(FILE_E, RANK_1);
constexpr Square SQUARE_E2 = makeSquare(FILE_E, RANK_2);
constexpr Square SQUARE_E3 = makeSquare(FILE_E, RANK_3);
constexpr Square SQUARE_E6 = makeSquare(FILE_E, RANK_6);
constexpr Square SQUARE_E8 = makeSquare(FILE_E, RANK_8);
constexpr Square SQUARE_A2 = makeSquare(FILE_A, RANK_2);
constexpr Square SQUARE_A7 = makeSquare(FILE_A, RANK_7);

TEST_CASE("FEN validation - valid positions", "[fen-validator]") {
    std::vector<std::string> validFens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "8/8/8/4k3/4K3/8/8/8 w - - 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"};

    for (const auto& fen : validFens) {
        Position pos = FenParser::parse(fen);
        std::string error;
        REQUIRE(FenValidator::validate(pos, &error));
    }
}

TEST_CASE("FEN validation - king count", "[fen-validator]") {
    SECTION("Missing white king") {
        Position pos;
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        std::string error;
        REQUIRE(!FenValidator::validateKings(pos, &error));
        REQUIRE(!error.empty());
    }

    SECTION("Missing black king") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        std::string error;
        REQUIRE(!FenValidator::validateKings(pos, &error));
        REQUIRE(!error.empty());
    }

    SECTION("Multiple white kings") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E2, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        std::string error;
        REQUIRE(!FenValidator::validateKings(pos, &error));
    }
}

TEST_CASE("FEN validation - pawn placement", "[fen-validator]") {
    SECTION("Pawn on rank 1") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setPiece(SQUARE_A1, Piece(PieceType::Pawn, Color::White));
        std::string error;
        REQUIRE(!FenValidator::validatePawns(pos, &error));
    }

    SECTION("Pawn on rank 8") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setPiece(SQUARE_A8, Piece(PieceType::Pawn, Color::Black));
        std::string error;
        REQUIRE(!FenValidator::validatePawns(pos, &error));
    }

    SECTION("Pawns on valid ranks") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setPiece(SQUARE_A2, Piece(PieceType::Pawn, Color::White));
        pos.setPiece(SQUARE_A7, Piece(PieceType::Pawn, Color::Black));
        std::string error;
        REQUIRE(FenValidator::validatePawns(pos, &error));
    }
}

TEST_CASE("FEN validation - en passant", "[fen-validator]") {
    SECTION("Valid en passant for white") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setSideToMove(Color::White);
        pos.setEnPassantSquare(makeSquare(FILE_E, RANK_6));
        std::string error;
        REQUIRE(FenValidator::validateEnPassant(pos, &error));
    }

    SECTION("Valid en passant for black") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setSideToMove(Color::Black);
        pos.setEnPassantSquare(makeSquare(FILE_E, RANK_3));
        std::string error;
        REQUIRE(FenValidator::validateEnPassant(pos, &error));
    }

    SECTION("Invalid en passant rank for white") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setSideToMove(Color::White);
        pos.setEnPassantSquare(makeSquare(FILE_E, RANK_3));
        std::string error;
        REQUIRE(!FenValidator::validateEnPassant(pos, &error));
    }

    SECTION("No en passant is valid") {
        Position pos;
        pos.setPiece(SQUARE_E1, Piece(PieceType::King, Color::White));
        pos.setPiece(SQUARE_E8, Piece(PieceType::King, Color::Black));
        pos.setEnPassantSquare(SQUARE_NONE);
        std::string error;
        REQUIRE(FenValidator::validateEnPassant(pos, &error));
    }
}
