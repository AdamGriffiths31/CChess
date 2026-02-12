#include "ai/MoveOrder.h"
#include "core/Board.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Position with a variety of captures available:
// White Qd1, Pd2, Nd4; Black Qd8, Rd5, Pb6
// FEN crafted so white has Nxd5(capture rook), Qxd5(capture rook), dxe3(if applicable), etc.
// Using a known tactical position is simpler.

// r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq -
// White has Qxf7+, Bxf7+ (captures pawn), etc.
static const char* TACTICAL_FEN =
    "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1";

TEST_CASE("MoveOrder: capturing a high-value piece scores higher", "[moveorder]") {
    Board board(TACTICAL_FEN);
    const Position& pos = board.position();

    // Qxf7 captures a pawn (value 100) with queen (value 900)
    // Bxf7 captures a pawn (value 100) with bishop (value 300)
    // Both capture the same victim, so the lower-value attacker should score higher
    Square f7 = makeSquare(FILE_F, RANK_7);
    Square h5 = makeSquare(FILE_H, RANK_5);
    Square c4 = makeSquare(FILE_C, RANK_4);

    Move qxf7(h5, f7, MoveType::Capture);
    Move bxf7(c4, f7, MoveType::Capture);

    int qScore = MoveOrder::score(qxf7, pos);
    int bScore = MoveOrder::score(bxf7, pos);

    // Same victim (pawn), but bishop is a lesser attacker -> higher MVV-LVA score
    REQUIRE(bScore > qScore);
}

TEST_CASE("MoveOrder: captures score higher than quiet moves", "[moveorder]") {
    Board board(TACTICAL_FEN);
    const Position& pos = board.position();

    Square f7 = makeSquare(FILE_F, RANK_7);
    Square c4 = makeSquare(FILE_C, RANK_4);
    Square d3 = makeSquare(FILE_D, RANK_3);

    Move bxf7(c4, f7, MoveType::Capture);  // capture
    Move bd3(c4, d3, MoveType::Normal);    // quiet

    REQUIRE(MoveOrder::score(bxf7, pos) > MoveOrder::score(bd3, pos));
}

TEST_CASE("MoveOrder: extractCaptures returns only captures and promotions", "[moveorder]") {
    Board board(TACTICAL_FEN);
    MoveList allMoves = board.getLegalMoves();

    Move captures[256];
    size_t n = MoveOrder::extractCaptures(allMoves, board.position(), captures, 256);

    // Every extracted move must be a capture or promotion
    for (size_t i = 0; i < n; ++i) {
        REQUIRE((captures[i].isCapture() || captures[i].isPromotion()));
    }

    // Count captures in the original list to verify none were lost
    size_t expected = 0;
    for (size_t i = 0; i < allMoves.size(); ++i) {
        if (allMoves[i].isCapture() || allMoves[i].isPromotion())
            ++expected;
    }
    REQUIRE(n == expected);
}

TEST_CASE("MoveOrder: extractCaptures returns moves sorted by MVV-LVA", "[moveorder]") {
    Board board(TACTICAL_FEN);
    MoveList allMoves = board.getLegalMoves();

    Move captures[256];
    size_t n = MoveOrder::extractCaptures(allMoves, board.position(), captures, 256);

    // Scores must be non-increasing
    for (size_t i = 1; i < n; ++i) {
        int prev = MoveOrder::score(captures[i - 1], board.position());
        int curr = MoveOrder::score(captures[i], board.position());
        REQUIRE(prev >= curr);
    }
}
