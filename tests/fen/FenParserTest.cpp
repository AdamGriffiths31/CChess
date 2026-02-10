#include "fen/FenParser.h"
#include "utils/Error.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Helper constants for tests
constexpr Square SQUARE_E1 = makeSquare(FILE_E, RANK_1);
constexpr Square SQUARE_E2 = makeSquare(FILE_E, RANK_2);
constexpr Square SQUARE_E3 = makeSquare(FILE_E, RANK_3);
constexpr Square SQUARE_E4 = makeSquare(FILE_E, RANK_4);
constexpr Square SQUARE_E8 = makeSquare(FILE_E, RANK_8);
constexpr Square SQUARE_A2 = makeSquare(FILE_A, RANK_2);

TEST_CASE("FEN parsing - starting position", "[fen]") {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos = FenParser::parse(fen);

    SECTION("Side to move") {
        REQUIRE(pos.sideToMove() == Color::White);
    }

    SECTION("Castling rights") {
        REQUIRE(pos.castlingRights() == ALL_CASTLING);
    }

    SECTION("En passant") {
        REQUIRE(pos.enPassantSquare() == SQUARE_NONE);
    }

    SECTION("Move counters") {
        REQUIRE(pos.halfmoveClock() == 0);
        REQUIRE(pos.fullmoveNumber() == 1);
    }

    SECTION("Pieces") {
        REQUIRE(pos.pieceAt(SQUARE_E1).type() == PieceType::King);
        REQUIRE(pos.pieceAt(SQUARE_E1).color() == Color::White);
        REQUIRE(pos.pieceAt(SQUARE_E8).type() == PieceType::King);
        REQUIRE(pos.pieceAt(SQUARE_E8).color() == Color::Black);
        REQUIRE(pos.pieceAt(SQUARE_A1).type() == PieceType::Rook);
        REQUIRE(pos.pieceAt(SQUARE_A2).type() == PieceType::Pawn);
    }
}

TEST_CASE("FEN parsing - after 1.e4", "[fen]") {
    std::string fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    Position pos = FenParser::parse(fen);

    REQUIRE(pos.sideToMove() == Color::Black);
    REQUIRE(pos.enPassantSquare() == makeSquare(FILE_E, RANK_3));
    REQUIRE(pos.pieceAt(makeSquare(FILE_E, RANK_4)).type() == PieceType::Pawn);
    REQUIRE(pos.pieceAt(makeSquare(FILE_E, RANK_2)).isEmpty());
}

TEST_CASE("FEN parsing - empty board", "[fen]") {
    std::string fen = "8/8/8/8/8/8/8/8 w - - 0 1";
    Position pos = FenParser::parse(fen);

    for (Square sq = 0; sq < 64; ++sq) {
        REQUIRE(pos.pieceAt(sq).isEmpty());
    }
}

TEST_CASE("FEN parsing - partial castling rights", "[fen]") {
    SECTION("White kingside only") {
        Position pos = FenParser::parse("8/8/8/8/8/8/8/8 w K - 0 1");
        REQUIRE(pos.castlingRights() == WHITE_KINGSIDE);
    }

    SECTION("Black queenside only") {
        Position pos = FenParser::parse("8/8/8/8/8/8/8/8 w q - 0 1");
        REQUIRE(pos.castlingRights() == BLACK_QUEENSIDE);
    }

    SECTION("Mixed rights") {
        Position pos = FenParser::parse("8/8/8/8/8/8/8/8 w Kq - 0 1");
        REQUIRE(pos.castlingRights() == (WHITE_KINGSIDE | BLACK_QUEENSIDE));
    }

    SECTION("No castling") {
        Position pos = FenParser::parse("8/8/8/8/8/8/8/8 w - - 0 1");
        REQUIRE(pos.castlingRights() == NO_CASTLING);
    }
}

TEST_CASE("FEN parsing - errors", "[fen]") {
    SECTION("Too few fields") {
        REQUIRE_THROWS_AS(FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"),
                          FenParseError);
    }

    SECTION("Invalid piece character") {
        REQUIRE_THROWS_AS(
            FenParser::parse("rnbqkXnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
            FenParseError);
    }

    SECTION("Wrong number of ranks") {
        REQUIRE_THROWS_AS(FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1"),
                          FenParseError);
    }

    SECTION("Rank sum not 8") {
        REQUIRE_THROWS_AS(
            FenParser::parse("rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
            FenParseError);
    }

    SECTION("Invalid side to move") {
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 x - - 0 1"), FenParseError);
    }

    SECTION("Invalid castling rights") {
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w KQkqX - 0 1"), FenParseError);
    }

    SECTION("Invalid en passant square") {
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w - z9 0 1"), FenParseError);
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w - e5 0 1"), FenParseError);
    }

    SECTION("Invalid halfmove clock") {
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w - - -1 1"), FenParseError);
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w - - abc 1"), FenParseError);
    }

    SECTION("Invalid fullmove number") {
        REQUIRE_THROWS_AS(FenParser::parse("8/8/8/8/8/8/8/8 w - - 0 0"), FenParseError);
    }
}

TEST_CASE("FEN serialization", "[fen]") {
    SECTION("Starting position") {
        std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        Position pos = FenParser::parse(fen);
        REQUIRE(FenParser::serialize(pos) == fen);
    }

    SECTION("Empty board") {
        std::string fen = "8/8/8/8/8/8/8/8 w - - 0 1";
        Position pos = FenParser::parse(fen);
        REQUIRE(FenParser::serialize(pos) == fen);
    }

    SECTION("Complex position") {
        std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
        Position pos = FenParser::parse(fen);
        REQUIRE(FenParser::serialize(pos) == fen);
    }
}

TEST_CASE("FEN round-trip", "[fen]") {
    std::vector<std::string> fens = {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                     "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
                                     "8/8/8/4k3/4K3/8/8/8 w - - 0 1",
                                     "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                                     "r3k2r/8/8/8/8/8/8/R3K2R b Kq - 10 50"};

    for (const auto& fen : fens) {
        Position pos = FenParser::parse(fen);
        REQUIRE(FenParser::serialize(pos) == fen);
    }
}
