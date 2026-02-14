#include "ai/Eval.h"
#include "core/Board.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;
using namespace cchess::eval;

// Helper: get pawn bitboards from a board
static std::pair<Bitboard, Bitboard> pawns(const Board& board) {
    const Position& pos = board.position();
    return {pos.pieces(PieceType::Pawn, Color::White), pos.pieces(PieceType::Pawn, Color::Black)};
}

// --- rookOpenFiles ---

TEST_CASE("Eval rookOpenFiles: rook on open file", "[eval]") {
    // White rook on e1, no pawns on e-file. Expected: +15
    Board board("7k/8/8/8/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(rookOpenFiles(board.position(), wp, bp) == 15);
}

TEST_CASE("Eval rookOpenFiles: rook on semi-open file", "[eval]") {
    // White rook on e1, no white pawn on e-file but black pawn on e5. Expected: +8
    Board board("7k/8/8/4p3/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(rookOpenFiles(board.position(), wp, bp) == 8);
}

TEST_CASE("Eval rookOpenFiles: rook on closed file", "[eval]") {
    // White rook on e1, white pawn on e4, black pawn on e5. Expected: 0
    Board board("7k/8/8/4p3/4P3/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(rookOpenFiles(board.position(), wp, bp) == 0);
}

TEST_CASE("Eval rookOpenFiles: both sides cancel out", "[eval]") {
    // White rook on e1 (open), black rook on e8 (open). Expected: 0
    Board board("4r2k/pppp1ppp/8/8/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(rookOpenFiles(board.position(), wp, bp) == 0);
}

TEST_CASE("Eval rookOpenFiles: two white rooks, one open one closed", "[eval]") {
    // White rooks on d1 (semi-open, black d-pawns) and e1 (open). Expected: 8 + 15 = 23
    Board board("7k/pppp1ppp/8/8/8/8/PPP2PPP/3RR2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(rookOpenFiles(board.position(), wp, bp) == 23);
}

// --- pawnStructure ---

TEST_CASE("Eval pawnStructure: doubled and isolated white pawns", "[eval]") {
    // Two white pawns on e-file (e2, e4), no pawns on d/f. Expected: -10 (doubled) + -30 (isolated
    // x2) = -40
    Board board("7k/8/8/8/4P3/8/4P3/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(pawnStructure(wp, bp) == -40);
}

TEST_CASE("Eval pawnStructure: isolated white pawn", "[eval]") {
    // White pawn on e4, no pawns on d/f files. Expected: -15
    Board board("7k/8/8/8/4P3/8/PPP3PP/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(pawnStructure(wp, bp) == -15);
}

TEST_CASE("Eval pawnStructure: clean structure", "[eval]") {
    // 8 white pawns, no doubled or isolated. Expected: 0
    Board board("7k/8/8/8/8/2P1P2P/PP1P1PP1/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(pawnStructure(wp, bp) == 0);
}

TEST_CASE("Eval pawnStructure: symmetric weaknesses cancel out", "[eval]") {
    // Both sides have doubled+isolated pawns on b and g files. Expected: 0
    Board board("7k/1p4p1/8/1p4p1/1P4P1/8/1P4P1/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(pawnStructure(wp, bp) == 0);
}

// --- passedPawns ---

TEST_CASE("Eval passedPawns: white passed pawn on e5", "[eval]") {
    // White pawn on e5 (rank 4) = +35. Black pawns a7/b7/c7/g7/h7 all passed (rank 6) = -5 each.
    // Total: 35 - 25 = 10
    Board board("7k/ppp3pp/8/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(passedPawns(wp, bp) == 10);
}

TEST_CASE("Eval passedPawns: blocked by pawn on same file", "[eval]") {
    // White e5 not passed (black e6 ahead). Black e6 not passed (white e5 below).
    // Black a7/b7/c7/g7/h7 passed (rank 6) = -5 each = -25. Total: -25
    Board board("7k/ppp3pp/4p3/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(passedPawns(wp, bp) == -25);
}

TEST_CASE("Eval passedPawns: blocked by pawn on adjacent file", "[eval]") {
    // White e5 blocked by black d6 — not passed. Black d6 not passed (white e5 on adj file below).
    // Black a7/b7/c7/g7/h7 passed (rank 6) = -5 each = -25. Total: -25
    Board board("7k/ppp3pp/3p4/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(passedPawns(wp, bp) == -25);
}

TEST_CASE("Eval passedPawns: both sides have passed pawns", "[eval]") {
    // White b4 passed (rank 3) = +20. Black f6 passed (rank 5) = -10. Total: +10
    Board board("7k/8/5p2/8/1P6/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    CHECK(passedPawns(wp, bp) == 10);
}

// --- bishopPair ---

TEST_CASE("Eval bishopPair: white has pair", "[eval]") {
    // White has 2 bishops, black has 1. Expected: +30
    Board board("7k/6b1/8/8/8/8/8/BB5K w - - 0 1");
    CHECK(bishopPair(board.position()) == 30);
}

TEST_CASE("Eval bishopPair: both sides have pair", "[eval]") {
    // Both sides have 2 bishops. Expected: 0
    Board board("7k/6bb/8/8/8/8/8/BB5K w - - 0 1");
    CHECK(bishopPair(board.position()) == 0);
}

TEST_CASE("Eval bishopPair: neither side has pair", "[eval]") {
    // One bishop each. Expected: 0
    Board board("7k/6b1/8/8/8/3B4/8/7K w - - 0 1");
    CHECK(bishopPair(board.position()) == 0);
}

TEST_CASE("Eval bishopPair: only black has pair", "[eval]") {
    // White has 1 bishop, black has 2. Expected: -30
    Board board("7k/1b4b1/8/8/8/3B4/8/7K w - - 0 1");
    CHECK(bishopPair(board.position()) == -30);
}

// --- mobility ---

TEST_CASE("Eval mobility: knight in center", "[eval]") {
    // Knight on d5, 8 moves available. Expected: 4 * (8 - 4) = 16
    Board board("7k/8/8/3N4/8/8/8/7K w - - 0 1");
    CHECK(mobility(board.position()) == 16);
}

TEST_CASE("Eval mobility: knight on edge", "[eval]") {
    // Knight on a8, 2 moves available. Expected: 4 * (2 - 4) = -8
    Board board("N6k/8/8/8/8/8/8/7K w - - 0 1");
    CHECK(mobility(board.position()) == -8);
}

TEST_CASE("Eval mobility: trapped bishop", "[eval]") {
    // Bishop on b2 hemmed in by pawns on a3/c3. 2 moves (a1, c1). Expected: 3 * (2 - 7) = -15
    Board board("7k/8/8/8/8/P1P5/1B6/7K w - - 0 1");
    CHECK(mobility(board.position()) == -15);
}

TEST_CASE("Eval mobility: open bishop", "[eval]") {
    // Bishop on c2, pawns on a3/c3 don't block diagonals. 9 moves. Expected: 3 * (9 - 7) = 6
    Board board("7k/8/8/8/8/P1P5/2B5/7K w - - 0 1");
    CHECK(mobility(board.position()) == 6);
}

TEST_CASE("Eval mobility: open rook", "[eval]") {
    // Rook e5 has 14 squares. Knights d6/f6 have 8 each.
    // Rook: 2*(14-7)=14. Knights: 4*(8-4)*2=32. Total: 46
    Board board("7k/8/3N1N2/4R3/8/3P1P2/8/7K w - - 0 1");
    CHECK(mobility(board.position()) == 46);
}

TEST_CASE("Eval mobility: closed rook", "[eval]") {
    // Rook g1 boxed in (1 square). Knight e1 trapped (1 square). Knights d6/f6 have 8 each.
    // Rook: 2*(1-7)=-12. Ne1: 4*(1-4)=-12. Nd6/Nf6: 4*(8-4)*2=32. Total: 8
    Board board("7k/8/3N1N2/8/8/3P1P2/6P1/4N1RK w - - 0 1");
    CHECK(mobility(board.position()) == 8);
}
