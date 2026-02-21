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
    // White rook on e1, no pawns on e-file
    Board board("7k/8/8/8/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = rookOpenFiles(board.position(), wp, bp);
    CHECK(s.mg == 15);
    CHECK(s.eg == 10);
}

TEST_CASE("Eval rookOpenFiles: rook on semi-open file", "[eval]") {
    // White rook on e1, no white pawn on e-file but black pawn on e5
    Board board("7k/8/8/4p3/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = rookOpenFiles(board.position(), wp, bp);
    CHECK(s.mg == 8);
    CHECK(s.eg == 5);
}

TEST_CASE("Eval rookOpenFiles: rook on closed file", "[eval]") {
    // White rook on e1, white pawn on e4, black pawn on e5
    Board board("7k/8/8/4p3/4P3/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = rookOpenFiles(board.position(), wp, bp);
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

TEST_CASE("Eval rookOpenFiles: both sides cancel out", "[eval]") {
    // White rook on e1 (open), black rook on e8 (open)
    Board board("4r2k/pppp1ppp/8/8/8/8/PPPP1PPP/4R2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = rookOpenFiles(board.position(), wp, bp);
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

TEST_CASE("Eval rookOpenFiles: two white rooks, one open one closed", "[eval]") {
    // White rooks on d1 (semi-open, black d-pawns) and e1 (open)
    Board board("7k/pppp1ppp/8/8/8/8/PPP2PPP/3RR2K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = rookOpenFiles(board.position(), wp, bp);
    CHECK(s.mg == 23);
    CHECK(s.eg == 15);
}

// --- pawnStructure ---

TEST_CASE("Eval pawnStructure: doubled and isolated white pawns", "[eval]") {
    // Two white pawns on e-file (e2, e4), no pawns on d/f
    // Doubled: S(-10,-15) * 1 = S(-10,-15). Isolated: S(-15,-20) * 2 = S(-30,-40). Total:
    // S(-40,-55)
    Board board("7k/8/8/8/4P3/8/4P3/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = pawnStructure(wp, bp);
    CHECK(s.mg == -40);
    CHECK(s.eg == -55);
}

TEST_CASE("Eval pawnStructure: isolated white pawn", "[eval]") {
    // White pawn on e4, no pawns on d/f files
    Board board("7k/8/8/8/4P3/8/PPP3PP/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = pawnStructure(wp, bp);
    CHECK(s.mg == -15);
    CHECK(s.eg == -20);
}

TEST_CASE("Eval pawnStructure: clean structure", "[eval]") {
    // 8 white pawns, no doubled or isolated
    Board board("7k/8/8/8/8/2P1P2P/PP1P1PP1/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = pawnStructure(wp, bp);
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

TEST_CASE("Eval pawnStructure: symmetric weaknesses cancel out", "[eval]") {
    // Both sides have doubled+isolated pawns on b and g files
    Board board("7k/1p4p1/8/1p4p1/1P4P1/8/1P4P1/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = pawnStructure(wp, bp);
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

// --- passedPawns ---

TEST_CASE("Eval passedPawns: white passed pawn on e5", "[eval]") {
    // White pawn on e5 (rank 4) = S(35,55). Black a7,b7,c7,g7,h7 passed rank 6 -> bonus[1]=S(5,10)
    // x5
    Board board("7k/ppp3pp/8/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = passedPawns(wp, bp);
    CHECK(s.mg == 10);
    CHECK(s.eg == 5);
}

TEST_CASE("Eval passedPawns: blocked by pawn on same file", "[eval]") {
    // White e5 not passed (black e6 ahead). Black e6 not passed (white e5 below).
    // Black a7,b7,c7,g7,h7 passed rank 6 -> bonus[1]=S(5,10) x5 = S(25,50)
    Board board("7k/ppp3pp/4p3/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = passedPawns(wp, bp);
    CHECK(s.mg == -25);
    CHECK(s.eg == -50);
}

TEST_CASE("Eval passedPawns: blocked by pawn on adjacent file", "[eval]") {
    // White e5 blocked by black d6. Black d6 not passed (white e5 on adj file).
    // Black a7,b7,c7,g7,h7 passed rank 6 -> bonus[1]=S(5,10) x5 = S(25,50)
    Board board("7k/ppp3pp/3p4/4P3/8/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = passedPawns(wp, bp);
    CHECK(s.mg == -25);
    CHECK(s.eg == -50);
}

TEST_CASE("Eval passedPawns: both sides have passed pawns", "[eval]") {
    // White b4 passed rank 3 = S(20,35). Black f6 rank 5 -> bonus[7-5]=bonus[2]=S(10,20)
    Board board("7k/8/5p2/8/1P6/8/8/7K w - - 0 1");
    auto [wp, bp] = pawns(board);
    Score s = passedPawns(wp, bp);
    CHECK(s.mg == 10);
    CHECK(s.eg == 15);
}

// --- bishopPair ---

TEST_CASE("Eval bishopPair: white has pair", "[eval]") {
    Board board("7k/6b1/8/8/8/8/8/BB5K w - - 0 1");
    Score s = bishopPair(board.position());
    CHECK(s.mg == 30);
    CHECK(s.eg == 40);
}

TEST_CASE("Eval bishopPair: both sides have pair", "[eval]") {
    Board board("7k/6bb/8/8/8/8/8/BB5K w - - 0 1");
    Score s = bishopPair(board.position());
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

TEST_CASE("Eval bishopPair: neither side has pair", "[eval]") {
    Board board("7k/6b1/8/8/8/3B4/8/7K w - - 0 1");
    Score s = bishopPair(board.position());
    CHECK(s.mg == 0);
    CHECK(s.eg == 0);
}

TEST_CASE("Eval bishopPair: only black has pair", "[eval]") {
    Board board("7k/1b4b1/8/8/8/3B4/8/7K w - - 0 1");
    Score s = bishopPair(board.position());
    CHECK(s.mg == -30);
    CHECK(s.eg == -40);
}

// --- mobility ---

TEST_CASE("Eval mobility: knight in center", "[eval]") {
    // Knight on d5, 8 moves available. (8-4)*S(4,4) = S(16,16)
    Board board("7k/8/8/3N4/8/8/8/7K w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == 16);
    CHECK(s.eg == 16);
}

TEST_CASE("Eval mobility: knight on edge", "[eval]") {
    // Knight on a8, 2 moves available. (2-4)*S(4,4) = S(-8,-8)
    Board board("N6k/8/8/8/8/8/8/7K w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == -8);
    CHECK(s.eg == -8);
}

TEST_CASE("Eval mobility: trapped bishop", "[eval]") {
    // Bishop on b2 hemmed in by pawns. 2 moves. (2-7)*S(3,3) = S(-15,-15)
    Board board("7k/8/8/8/8/P1P5/1B6/7K w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == -15);
    CHECK(s.eg == -15);
}

TEST_CASE("Eval mobility: open bishop", "[eval]") {
    // Bishop on c2, 9 moves. (9-7)*S(3,3) = S(6,6)
    Board board("7k/8/8/8/8/P1P5/2B5/7K w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == 6);
    CHECK(s.eg == 6);
}

TEST_CASE("Eval mobility: open rook", "[eval]") {
    // Rook e5 14 squares, knights d6/f6 8 each
    // Rook: (14-7)*S(2,2)=S(14,14). Knights: (8-4)*S(4,4)*2=S(32,32). Total: S(46,46)
    Board board("7k/8/3N1N2/4R3/8/3P1P2/8/7K w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == 46);
    CHECK(s.eg == 46);
}

TEST_CASE("Eval mobility: closed rook", "[eval]") {
    // Rook g1 boxed in (1 sq), Knight e1 (1 sq), Knights d6/f6 (8 each)
    // Rook: (1-7)*S(2,2)=S(-12,-12). Ne1: (1-4)*S(4,4)=S(-12,-12). Nd6/Nf6:
    // (8-4)*S(4,4)*2=S(32,32). Total: S(8,8)
    Board board("7k/8/3N1N2/8/8/3P1P2/6P1/4N1RK w - - 0 1");
    Score s = mobility(board.position());
    CHECK(s.mg == 8);
    CHECK(s.eg == 8);
}
