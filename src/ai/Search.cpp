#include "ai/Search.h"

#include "ai/Eval.h"
#include "ai/MoveOrder.h"

namespace cchess {

// ============================================================================
// Search features:
//   - Iterative deepening with time control
//   - Negamax + alpha-beta pruning
//   - Quiescence search (captures + promotions) to eliminate the horizon effect
//   - MVV-LVA capture ordering (see MoveOrder.cpp)
// ============================================================================

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

        MoveOrder::sort(moves, board_.position());

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
        return quiescence(alpha, beta, ply);

    MoveOrder::sort(moves, board_.position());

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
// Quiescence search -- only searches captures and promotions so that the
// static evaluation is never called in a tactically unstable position.
// Uses "stand pat" as a lower bound: the side to move can always choose
// not to capture.
// ============================================================================

int Search::quiescence(int alpha, int beta, int ply) {
    if ((nodes_ & 1023) == 0)
        checkTime();
    if (stopped_)
        return 0;

    int standPat = eval::evaluate(board_.position());

    // Stand-pat cutoff: side to move can choose not to capture
    if (standPat >= beta)
        return beta;
    if (standPat > alpha)
        alpha = standPat;

    MoveList allMoves = board_.getLegalMoves();

    Move captures[256];
    size_t numCaptures = MoveOrder::extractCaptures(allMoves, board_.position(), captures, 256);

    for (size_t i = 0; i < numCaptures; ++i) {
        UndoInfo undo = board_.makeMoveUnchecked(captures[i]);
        ++nodes_;
        int score = -quiescence(-beta, -alpha, ply + 1);
        board_.unmakeMove(captures[i], undo);

        if (stopped_)
            return 0;

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

// ============================================================================
// Helpers
// ============================================================================

void Search::checkTime() {
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    if (elapsed >= config_.searchTime) {
        stopped_ = true;
    }
}

}  // namespace cchess
