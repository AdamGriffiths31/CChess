#include "core/Bitboard.h"
#include "core/Position.h"
#include "fen/FenParser.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// ============================================================
// Bitboard constants
// ============================================================

TEST_CASE("Bitboard constants", "[bitboard]") {
    SECTION("BB_EMPTY and BB_ALL") {
        REQUIRE(BB_EMPTY == 0ULL);
        REQUIRE(BB_ALL == ~0ULL);
        REQUIRE(popCount(BB_EMPTY) == 0);
        REQUIRE(popCount(BB_ALL) == 64);
    }

    SECTION("File bitboards have 8 bits each") {
        REQUIRE(popCount(FILE_A_BB) == 8);
        REQUIRE(popCount(FILE_H_BB) == 8);
        // Files should not overlap
        REQUIRE((FILE_A_BB & FILE_B_BB) == BB_EMPTY);
        REQUIRE((FILE_A_BB & FILE_H_BB) == BB_EMPTY);
    }

    SECTION("Rank bitboards have 8 bits each") {
        REQUIRE(popCount(RANK_1_BB) == 8);
        REQUIRE(popCount(RANK_8_BB) == 8);
        // Ranks should not overlap
        REQUIRE((RANK_1_BB & RANK_2_BB) == BB_EMPTY);
        REQUIRE((RANK_1_BB & RANK_8_BB) == BB_EMPTY);
    }

    SECTION("All files cover entire board") {
        Bitboard allFiles = FILE_A_BB | FILE_B_BB | FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB |
                            FILE_G_BB | FILE_H_BB;
        REQUIRE(allFiles == BB_ALL);
    }

    SECTION("All ranks cover entire board") {
        Bitboard allRanks = RANK_1_BB | RANK_2_BB | RANK_3_BB | RANK_4_BB | RANK_5_BB | RANK_6_BB |
                            RANK_7_BB | RANK_8_BB;
        REQUIRE(allRanks == BB_ALL);
    }

    SECTION("File-rank intersection is a single square") {
        // a1 = file A & rank 1
        REQUIRE(popCount(FILE_A_BB & RANK_1_BB) == 1);
        REQUIRE(testBit(FILE_A_BB & RANK_1_BB, SQUARE_A1));
        // h8 = file H & rank 8
        REQUIRE(popCount(FILE_H_BB & RANK_8_BB) == 1);
        REQUIRE(testBit(FILE_H_BB & RANK_8_BB, SQUARE_H8));
    }
}

// ============================================================
// Bit manipulation
// ============================================================

TEST_CASE("squareBB", "[bitboard]") {
    REQUIRE(squareBB(SQUARE_A1) == 1ULL);
    REQUIRE(squareBB(SQUARE_H1) == (1ULL << 7));
    REQUIRE(squareBB(SQUARE_A8) == (1ULL << 56));
    REQUIRE(squareBB(SQUARE_H8) == (1ULL << 63));
}

TEST_CASE("testBit", "[bitboard]") {
    Bitboard b = squareBB(makeSquare(FILE_E, RANK_4));
    REQUIRE(testBit(b, makeSquare(FILE_E, RANK_4)));
    REQUIRE_FALSE(testBit(b, makeSquare(FILE_D, RANK_4)));
    REQUIRE_FALSE(testBit(b, SQUARE_A1));
}

TEST_CASE("setBit and clearBit", "[bitboard]") {
    Bitboard b = BB_EMPTY;
    setBit(b, SQUARE_A1);
    REQUIRE(testBit(b, SQUARE_A1));
    REQUIRE(popCount(b) == 1);

    setBit(b, SQUARE_H8);
    REQUIRE(popCount(b) == 2);

    clearBit(b, SQUARE_A1);
    REQUIRE_FALSE(testBit(b, SQUARE_A1));
    REQUIRE(popCount(b) == 1);
}

TEST_CASE("popCount", "[bitboard]") {
    REQUIRE(popCount(BB_EMPTY) == 0);
    REQUIRE(popCount(squareBB(0)) == 1);
    REQUIRE(popCount(RANK_1_BB) == 8);
    REQUIRE(popCount(BB_ALL) == 64);
    REQUIRE(popCount(0x5555555555555555ULL) == 32);
}

TEST_CASE("lsb and msb", "[bitboard]") {
    SECTION("Single bit") {
        REQUIRE(lsb(squareBB(0)) == 0);
        REQUIRE(lsb(squareBB(63)) == 63);
        REQUIRE(msb(squareBB(0)) == 0);
        REQUIRE(msb(squareBB(63)) == 63);
    }

    SECTION("Multiple bits") {
        Bitboard b = squareBB(10) | squareBB(30) | squareBB(50);
        REQUIRE(lsb(b) == 10);
        REQUIRE(msb(b) == 50);
    }
}

TEST_CASE("popLsb", "[bitboard]") {
    Bitboard b = squareBB(5) | squareBB(20) | squareBB(40);
    REQUIRE(popCount(b) == 3);

    Square s1 = popLsb(b);
    REQUIRE(s1 == 5);
    REQUIRE(popCount(b) == 2);

    Square s2 = popLsb(b);
    REQUIRE(s2 == 20);
    REQUIRE(popCount(b) == 1);

    Square s3 = popLsb(b);
    REQUIRE(s3 == 40);
    REQUIRE(b == BB_EMPTY);
}

TEST_CASE("moreThanOne", "[bitboard]") {
    REQUIRE_FALSE(moreThanOne(BB_EMPTY));
    REQUIRE_FALSE(moreThanOne(squareBB(0)));
    REQUIRE(moreThanOne(squareBB(0) | squareBB(1)));
    REQUIRE(moreThanOne(BB_ALL));
}

// ============================================================
// Shift operations
// ============================================================

TEST_CASE("Shift operations", "[bitboard]") {
    SECTION("shiftNorth") {
        Bitboard b = RANK_1_BB;
        REQUIRE(shiftNorth(b) == RANK_2_BB);
        // Rank 8 shifts off the board
        REQUIRE(shiftNorth(RANK_8_BB) == BB_EMPTY);
    }

    SECTION("shiftSouth") {
        Bitboard b = RANK_2_BB;
        REQUIRE(shiftSouth(b) == RANK_1_BB);
        REQUIRE(shiftSouth(RANK_1_BB) == BB_EMPTY);
    }

    SECTION("shiftEast does not wrap") {
        Bitboard h1 = squareBB(SQUARE_H1);
        REQUIRE(shiftEast(h1) == BB_EMPTY);
        Bitboard a1 = squareBB(SQUARE_A1);
        REQUIRE(shiftEast(a1) == squareBB(makeSquare(FILE_B, RANK_1)));
    }

    SECTION("shiftWest does not wrap") {
        Bitboard a1 = squareBB(SQUARE_A1);
        REQUIRE(shiftWest(a1) == BB_EMPTY);
        Bitboard b1 = squareBB(makeSquare(FILE_B, RANK_1));
        REQUIRE(shiftWest(b1) == squareBB(SQUARE_A1));
    }

    SECTION("Diagonal shifts") {
        Bitboard e4 = squareBB(makeSquare(FILE_E, RANK_4));
        REQUIRE(shiftNorthEast(e4) == squareBB(makeSquare(FILE_F, RANK_5)));
        REQUIRE(shiftNorthWest(e4) == squareBB(makeSquare(FILE_D, RANK_5)));
        REQUIRE(shiftSouthEast(e4) == squareBB(makeSquare(FILE_F, RANK_3)));
        REQUIRE(shiftSouthWest(e4) == squareBB(makeSquare(FILE_D, RANK_3)));
    }

    SECTION("Diagonal shifts on edges do not wrap") {
        Bitboard a1 = squareBB(SQUARE_A1);
        REQUIRE(shiftSouthWest(a1) == BB_EMPTY);
        REQUIRE(shiftSouthEast(a1) == BB_EMPTY);
        REQUIRE(shiftNorthWest(a1) == BB_EMPTY);

        Bitboard h8 = squareBB(SQUARE_H8);
        REQUIRE(shiftNorthEast(h8) == BB_EMPTY);
        REQUIRE(shiftNorthWest(h8) == BB_EMPTY);
        REQUIRE(shiftSouthEast(h8) == BB_EMPTY);
    }
}

// ============================================================
// Position bitboard synchronization
// ============================================================

TEST_CASE("Position bitboards are empty on construction", "[bitboard][position]") {
    Position pos;
    REQUIRE(pos.occupied() == BB_EMPTY);
    REQUIRE(pos.pieces(Color::White) == BB_EMPTY);
    REQUIRE(pos.pieces(Color::Black) == BB_EMPTY);
    REQUIRE(pos.pieces(PieceType::Pawn) == BB_EMPTY);
    REQUIRE(pos.pieces(PieceType::King) == BB_EMPTY);
}

TEST_CASE("setPiece updates bitboards", "[bitboard][position]") {
    Position pos;
    Square e4 = makeSquare(FILE_E, RANK_4);

    pos.setPiece(e4, Piece(PieceType::Pawn, Color::White));

    REQUIRE(testBit(pos.occupied(), e4));
    REQUIRE(testBit(pos.pieces(Color::White), e4));
    REQUIRE(testBit(pos.pieces(PieceType::Pawn), e4));
    REQUIRE(testBit(pos.pieces(PieceType::Pawn, Color::White), e4));

    REQUIRE_FALSE(testBit(pos.pieces(Color::Black), e4));
    REQUIRE_FALSE(testBit(pos.pieces(PieceType::Knight), e4));
}

TEST_CASE("clearSquare updates bitboards", "[bitboard][position]") {
    Position pos;
    Square e4 = makeSquare(FILE_E, RANK_4);

    pos.setPiece(e4, Piece(PieceType::Rook, Color::Black));
    REQUIRE(testBit(pos.occupied(), e4));

    pos.clearSquare(e4);
    REQUIRE_FALSE(testBit(pos.occupied(), e4));
    REQUIRE_FALSE(testBit(pos.pieces(Color::Black), e4));
    REQUIRE_FALSE(testBit(pos.pieces(PieceType::Rook), e4));
}

TEST_CASE("setPiece replacing a piece updates bitboards correctly", "[bitboard][position]") {
    Position pos;
    Square e4 = makeSquare(FILE_E, RANK_4);

    pos.setPiece(e4, Piece(PieceType::Pawn, Color::White));
    pos.setPiece(e4, Piece(PieceType::Knight, Color::Black));

    // Old piece should be gone
    REQUIRE_FALSE(testBit(pos.pieces(PieceType::Pawn), e4));
    REQUIRE_FALSE(testBit(pos.pieces(Color::White), e4));

    // New piece should be present
    REQUIRE(testBit(pos.pieces(PieceType::Knight), e4));
    REQUIRE(testBit(pos.pieces(Color::Black), e4));
    REQUIRE(testBit(pos.occupied(), e4));
}

TEST_CASE("clear() resets all bitboards", "[bitboard][position]") {
    Position pos;
    pos.setPiece(SQUARE_A1, Piece(PieceType::Rook, Color::White));
    pos.setPiece(SQUARE_H8, Piece(PieceType::King, Color::Black));

    pos.clear();
    REQUIRE(pos.occupied() == BB_EMPTY);
    REQUIRE(pos.pieces(Color::White) == BB_EMPTY);
    REQUIRE(pos.pieces(Color::Black) == BB_EMPTY);
}

// ============================================================
// FEN parsing bitboard consistency
// ============================================================

TEST_CASE("Starting position bitboard consistency", "[bitboard][fen]") {
    Position pos = FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    SECTION("Piece counts") {
        REQUIRE(popCount(pos.pieces(Color::White)) == 16);
        REQUIRE(popCount(pos.pieces(Color::Black)) == 16);
        REQUIRE(popCount(pos.occupied()) == 32);
    }

    SECTION("Piece type counts") {
        REQUIRE(popCount(pos.pieces(PieceType::Pawn)) == 16);
        REQUIRE(popCount(pos.pieces(PieceType::Knight)) == 4);
        REQUIRE(popCount(pos.pieces(PieceType::Bishop)) == 4);
        REQUIRE(popCount(pos.pieces(PieceType::Rook)) == 4);
        REQUIRE(popCount(pos.pieces(PieceType::Queen)) == 2);
        REQUIRE(popCount(pos.pieces(PieceType::King)) == 2);
    }

    SECTION("Per-side piece counts") {
        REQUIRE(popCount(pos.pieces(PieceType::Pawn, Color::White)) == 8);
        REQUIRE(popCount(pos.pieces(PieceType::Pawn, Color::Black)) == 8);
        REQUIRE(popCount(pos.pieces(PieceType::King, Color::White)) == 1);
        REQUIRE(popCount(pos.pieces(PieceType::King, Color::Black)) == 1);
    }

    SECTION("White pawns on rank 2") {
        REQUIRE((pos.pieces(PieceType::Pawn, Color::White) & RANK_2_BB) ==
                pos.pieces(PieceType::Pawn, Color::White));
    }

    SECTION("Black pawns on rank 7") {
        REQUIRE((pos.pieces(PieceType::Pawn, Color::Black) & RANK_7_BB) ==
                pos.pieces(PieceType::Pawn, Color::Black));
    }

    SECTION("Bitboards match pieceAt for every square") {
        for (Square sq = 0; sq < 64; ++sq) {
            const Piece& piece = pos.pieceAt(sq);
            bool occupied = testBit(pos.occupied(), sq);
            REQUIRE(occupied == !piece.isEmpty());

            if (!piece.isEmpty()) {
                REQUIRE(testBit(pos.pieces(piece.type()), sq));
                REQUIRE(testBit(pos.pieces(piece.color()), sq));
                REQUIRE(testBit(pos.pieces(piece.type(), piece.color()), sq));
            }
        }
    }
}

TEST_CASE("Kiwipete position bitboard consistency", "[bitboard][fen]") {
    Position pos =
        FenParser::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    // Verify every square matches between array and bitboards
    for (Square sq = 0; sq < 64; ++sq) {
        const Piece& piece = pos.pieceAt(sq);
        bool occupied = testBit(pos.occupied(), sq);
        REQUIRE(occupied == !piece.isEmpty());

        if (!piece.isEmpty()) {
            REQUIRE(testBit(pos.pieces(piece.type()), sq));
            REQUIRE(testBit(pos.pieces(piece.color()), sq));
        }
    }

    // Colors should be disjoint
    REQUIRE((pos.pieces(Color::White) & pos.pieces(Color::Black)) == BB_EMPTY);

    // Occupied should be union of colors
    REQUIRE(pos.occupied() == (pos.pieces(Color::White) | pos.pieces(Color::Black)));
}
