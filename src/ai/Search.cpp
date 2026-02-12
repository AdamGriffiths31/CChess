#include "ai/Search.h"

#include "ai/Eval.h"

#include <algorithm>

namespace cchess {

// ============================================================================
// Construction
// ============================================================================

Search::Search(const Board& board, const SearchConfig& config)
    : board_(board), config_(config), stopped_(false), nodes_(0) {}

// ============================================================================
// Search
// ============================================================================

Move Search::findBestMove() {
    startTime_ = std::chrono::steady_clock::now();
    stopped_ = false;
    nodes_ = 0;

    Move bestMove;

    for (int depth = 1; depth <= config_.maxDepth; ++depth) {
        int alpha = -eval::SCORE_INFINITY;
        int beta = eval::SCORE_INFINITY;
        Move depthBest;
        int bestScore = -eval::SCORE_INFINITY;

        MoveList moves = board_.getLegalMoves();
        if (moves.empty())
            break;

        orderMoves(moves);

        for (size_t i = 0; i < moves.size(); ++i) {
            UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
            ++nodes_;
            int score = -negamax(depth - 1, -beta, -alpha, 1);
            board_.unmakeMove(moves[i], undo);

            if (stopped_)
                break;

            if (score > bestScore) {
                bestScore = score;
                depthBest = moves[i];
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        if (stopped_)
            break;

        bestMove = depthBest;

        // Stop early on forced mate
        if (bestScore >= eval::SCORE_MATE - config_.maxDepth)
            break;
    }

    return bestMove;
}

int Search::negamax(int depth, int alpha, int beta, int ply) {
    if ((nodes_ & 1023) == 0)
        checkTime();
    if (stopped_)
        return 0;

    MoveList moves = board_.getLegalMoves();

    if (moves.empty()) {
        if (board_.isInCheck()) {
            return -(eval::SCORE_MATE - ply);  // Checkmate
        }
        return eval::SCORE_DRAW;  // Stalemate
    }

    if (board_.isDraw())
        return eval::SCORE_DRAW;

    if (depth == 0)
        return eval::evaluate(board_.position());

    orderMoves(moves);

    int bestScore = -eval::SCORE_INFINITY;

    for (size_t i = 0; i < moves.size(); ++i) {
        UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
        ++nodes_;
        int score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        board_.unmakeMove(moves[i], undo);

        if (stopped_)
            return 0;

        if (score > bestScore)
            bestScore = score;
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    return bestScore;
}

// ============================================================================
// Helpers
// ============================================================================

void Search::orderMoves(MoveList& moves) {
    std::stable_partition(moves.begin(), moves.end(), [](const Move& m) { return m.isCapture(); });
}

void Search::checkTime() {
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    if (elapsed >= config_.searchTime) {
        stopped_ = true;
    }
}

}  // namespace cchess
