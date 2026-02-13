#include "ai/Search.h"

#include "ai/Eval.h"
#include "ai/MoveOrder.h"

#include <cassert>

namespace cchess {

// ============================================================================
// Search features:
//   - Iterative deepening with time control
//   - Negamax + alpha-beta pruning
//   - Quiescence search (captures + promotions) to eliminate the horizon effect
//   - Transposition table with best-move ordering
//   - MVV-LVA capture ordering (see MoveOrder.cpp)
// ============================================================================

// ============================================================================
// Construction
// ============================================================================

Search::Search(const Board& board, const SearchConfig& config, TranspositionTable& tt)
    : board_(board), config_(config), tt_(tt), stopped_(false), nodes_(0) {}

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

        // Probe TT at root for move ordering hint
        Move ttMove;
        TTEntry ttEntry;
        if (tt_.probe(board_.position().hash(), ttEntry))
            ttMove = ttEntry.bestMove;

        MoveOrder::sort(moves, board_.position(), ttMove);

        for (size_t i = 0; i < moves.size(); ++i) {
            UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
            ++nodes_;

            int score;
            if (i == 0) {
                score = -negamax(depth - 1, -beta, -alpha, 1);
            } else {
                score = -negamax(depth - 1, -alpha - 1, -alpha, 1);
                if (score > alpha && score < beta)
                    score = -negamax(depth - 1, -beta, -alpha, 1);
            }

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

        // Store root result in TT
        tt_.store(board_.position().hash(), scoreToTT(bestScore, 0), depth, TTBound::EXACT,
                  bestMove);

        // Stop early on forced mate
        if (bestScore >= eval::SCORE_MATE - config_.maxDepth)
            break;
    }

    return bestMove;
}

int Search::negamax(int depth, int alpha, int beta, int ply) {
    assert(alpha < beta);
    assert(depth >= 0);
    assert(ply >= 0);

    if ((nodes_ & 1023) == 0)
        checkTime();
    if (stopped_)
        return 0;

    // TT probe
    Move ttMove;
    uint64_t posHash = board_.position().hash();
    TTEntry ttEntry;
    if (tt_.probe(posHash, ttEntry)) {
        ttMove = ttEntry.bestMove;
        if (ttEntry.depth >= depth) {
            bool isPvNode = (beta - alpha > 1);
            if (!isPvNode) {
                int ttScore = scoreFromTT(ttEntry.score, ply);
                switch (ttEntry.bound) {
                    case TTBound::EXACT:
                        ++tt_.stats().cutoffs;
                        return ttScore;
                    case TTBound::LOWER:
                        if (ttScore >= beta) {
                            ++tt_.stats().cutoffs;
                            return ttScore;
                        }
                        break;
                    case TTBound::UPPER:
                        if (ttScore <= alpha) {
                            ++tt_.stats().cutoffs;
                            return ttScore;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

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

    MoveOrder::sort(moves, board_.position(), ttMove);

    int bestScore = -eval::SCORE_INFINITY;
    Move bestMoveInNode;
    int origAlpha = alpha;

    for (size_t i = 0; i < moves.size(); ++i) {
        UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
        ++nodes_;

        int score;
        if (i == 0) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        } else {
            score = -negamax(depth - 1, -alpha - 1, -alpha, ply + 1);
            if (score > alpha && score < beta)
                score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        }

        board_.unmakeMove(moves[i], undo);

        if (stopped_)
            return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMoveInNode = moves[i];
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    // Determine bound type and store
    if (!stopped_) {
        TTBound bound;
        if (alpha >= beta)
            bound = TTBound::LOWER;
        else if (bestScore > origAlpha)
            bound = TTBound::EXACT;
        else
            bound = TTBound::UPPER;

        tt_.store(posHash, scoreToTT(bestScore, ply), depth, bound, bestMoveInNode);
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
    assert(alpha < beta);
    assert(ply >= 0);

    if ((nodes_ & 1023) == 0)
        checkTime();
    if (stopped_)
        return 0;

    // TT probe
    uint64_t posHash = board_.position().hash();
    TTEntry ttEntry;
    if (tt_.probe(posHash, ttEntry)) {
        int ttScore = scoreFromTT(ttEntry.score, ply);
        if (ttEntry.bound == TTBound::EXACT) {
            ++tt_.stats().cutoffs;
            return ttScore;
        }
        if (ttEntry.bound == TTBound::LOWER && ttScore >= beta) {
            ++tt_.stats().cutoffs;
            return ttScore;
        }
        if (ttEntry.bound == TTBound::UPPER && ttScore <= alpha) {
            ++tt_.stats().cutoffs;
            return ttScore;
        }
    }

    int standPat = eval::evaluate(board_.position());

    // Stand-pat cutoff: side to move can choose not to capture
    if (standPat >= beta)
        return beta;

    int origAlpha = alpha;
    int bestScore = standPat;

    if (standPat > alpha)
        alpha = standPat;
    Move bestMoveInNode;

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

        if (score > bestScore) {
            bestScore = score;
            bestMoveInNode = captures[i];
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break;
    }

    // Store in TT with depth 0 (qsearch marker)
    if (!stopped_) {
        TTBound bound;
        if (bestScore >= beta)
            bound = TTBound::LOWER;
        else if (bestScore > origAlpha)
            bound = TTBound::EXACT;
        else
            bound = TTBound::UPPER;

        tt_.store(posHash, scoreToTT(bestScore, ply), 0, bound, bestMoveInNode);
    }

    return bestScore;
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
