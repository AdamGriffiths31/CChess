#include "ai/PawnTable.h"
#include "ai/Search.h"
#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Board.h"
#include "core/Move.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Runs a fixed-depth search on the given FEN and returns the best move.
static Move bestMove(const std::string& fen, int depth) {
    Board board(fen);
    SearchConfig config;
    config.maxDepth = depth;
    config.searchTime = std::chrono::milliseconds(10000);
    TranspositionTable tt(16);
    eval::PawnTable pt;
    Search search(board, config, tt, pt);
    return search.findBestMove();
}

// Checks only from/to squares, ignoring move type flags (capture, normal, etc.)
static bool movesSquares(Move m, const std::string& from, const std::string& to) {
    Square f = makeSquare(static_cast<File>(from[0] - 'a'), static_cast<Rank>(from[1] - '1'));
    Square t = makeSquare(static_cast<File>(to[0] - 'a'), static_cast<Rank>(to[1] - '1'));
    return m.from() == f && m.to() == t;
}

TEST_CASE("Search: mate in 1 — back rank", "[search]") {
    // Rook delivers back-rank checkmate: Rc8#
    Move best = bestMove("7k/5ppp/8/8/8/8/2R5/7K w - - 0 1", 4);
    CHECK(movesSquares(best, "c2", "c8"));
}

TEST_CASE("Search: winning capture — queen en prise", "[search]") {
    // Black queen on h4 is undefended; Nxh4 wins a queen for a knight.
    Move best = bestMove("rnb1kbnr/pppp1ppp/8/4p3/4P2q/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", 4);
    CHECK(movesSquares(best, "f3", "h4"));
}

TEST_CASE("Search: knight fork — wins rook", "[search]") {
    // Nf6+ forks king on d5 and rook on g4; after king moves Nxg4 wins the rook.
    Move best = bestMove("8/3N4/8/3k4/6r1/8/8/4K3 w - - 0 1", 4);
    CHECK(movesSquares(best, "d7", "f6"));
}

TEST_CASE("Search: rook skewer — wins queen", "[search]") {
    // Rg4+ skewers king on g6 through to queen on g8; after king moves Rxg8 wins the queen.
    Move best = bestMove("6q1/8/6k1/8/7R/8/8/K7 w - - 0 1", 4);
    CHECK(movesSquares(best, "h4", "g4"));
}
