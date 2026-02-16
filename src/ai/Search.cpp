#include "ai/Search.h"

#include "ai/Eval.h"
#include "ai/MoveOrder.h"

#include <cassert>
#include <cmath>
#include <unordered_set>

namespace cchess {

// ============================================================================
// LMR table initialization
// ============================================================================

std::array<std::array<int, Search::MAX_LMR_MOVES>, Search::MAX_LMR_DEPTH> Search::lmrTable_;
bool Search::lmrInitialized_ = false;

void Search::initLMR() {
    if (lmrInitialized_)
        return;
    constexpr double K = 2.0;
    for (size_t d = 0; d < MAX_LMR_DEPTH; ++d) {
        for (size_t m = 0; m < MAX_LMR_MOVES; ++m) {
            if (d == 0 || m == 0)
                lmrTable_[d][m] = 0;
            else
                lmrTable_[d][m] = static_cast<int>(std::log(static_cast<double>(d)) *
                                                   std::log(static_cast<double>(m)) / K);
        }
    }
    lmrInitialized_ = true;
}

// ============================================================================
// Search features:
//   - Iterative deepening with time control
//   - Negamax + alpha-beta pruning
//   - Principal Variation Search (PVS)
//   - Late Move Reductions (LMR)
//   - Quiescence search (captures + promotions) to eliminate the horizon effect
//   - Transposition table with best-move ordering
//   - MVV-LVA capture ordering (see MoveOrder.cpp)
// ============================================================================

// ============================================================================
// Construction
// ============================================================================

Search::Search(const Board& board, const SearchConfig& config, TranspositionTable& tt,
               InfoCallback infoCallback)
    : board_(board),
      config_(config),
      tt_(tt),
      infoCallback_(std::move(infoCallback)),
      stopped_(false),
      nodes_(0) {
    initLMR();
}

// ============================================================================
// PV extraction from TT
// ============================================================================

std::vector<Move> Search::extractPV(int maxLength) {
    std::vector<Move> pv;
    std::vector<UndoInfo> undos;
    std::unordered_set<uint64_t> seen;

    for (int i = 0; i < maxLength; ++i) {
        uint64_t hash = board_.position().hash();
        if (seen.count(hash))
            break;
        seen.insert(hash);

        TTEntry entry;
        if (!tt_.probe(hash, entry) || entry.bestMove.isNull())
            break;

        // Verify move is legal
        MoveList legal = board_.getLegalMoves();
        bool found = false;
        for (size_t j = 0; j < legal.size(); ++j) {
            if (legal[j] == entry.bestMove) {
                found = true;
                break;
            }
        }
        if (!found)
            break;

        pv.push_back(entry.bestMove);
        undos.push_back(board_.makeMoveUnchecked(entry.bestMove));
    }

    // Undo all PV moves in reverse
    for (size_t i = pv.size(); i > 0; --i) {
        board_.unmakeMove(pv[i - 1], undos[i - 1]);
    }

    return pv;
}

// ============================================================================
// Search
// ============================================================================

Move Search::findBestMove() {
    startTime_ = std::chrono::steady_clock::now();
    stopped_ = false;
    nodes_ = 0;
    tt_.newSearch();
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

        // Report search info
        if (infoCallback_) {
            auto elapsed = std::chrono::steady_clock::now() - startTime_;
            int timeMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

            SearchInfo info;
            info.depth = depth;
            info.score = bestScore;
            info.nodes = nodes_;
            info.timeMs = timeMs;
            info.pv = extractPV(depth);
            infoCallback_(info);
        }

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
                switch (ttEntry.bound()) {
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
    bool inCheck = board_.isInCheck();

    for (size_t i = 0; i < moves.size(); ++i) {
        UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
        ++nodes_;

        bool givesCheck = board_.isInCheck();

        int score;
        if (i == 0) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        } else {
            // Late Move Reduction
            int reduction = 0;
            if (depth >= 3 && i >= 2 && !inCheck && !givesCheck && !moves[i].isCapture() &&
                !moves[i].isPromotion()) {
                size_t di = static_cast<size_t>(std::min(depth, MAX_LMR_DEPTH - 1));
                size_t mi = std::min(i, static_cast<size_t>(MAX_LMR_MOVES - 1));
                reduction = lmrTable_[di][mi];
                // Don't reduce into qsearch
                if (reduction >= depth - 1)
                    reduction = depth - 2;
            }

            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1);

            // Re-search at full depth if reduced search beat alpha
            if (reduction > 0 && score > alpha)
                score = -negamax(depth - 1, -alpha - 1, -alpha, ply + 1);

            // PVS re-search with full window
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
        if (ttEntry.bound() == TTBound::EXACT) {
            ++tt_.stats().cutoffs;
            return ttScore;
        }
        if (ttEntry.bound() == TTBound::LOWER && ttScore >= beta) {
            ++tt_.stats().cutoffs;
            return ttScore;
        }
        if (ttEntry.bound() == TTBound::UPPER && ttScore <= alpha) {
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

    MoveList captures = board_.getLegalCaptures();
    MoveOrder::sort(captures, board_.position());

    for (size_t i = 0; i < captures.size(); ++i) {
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
    if (config_.stopSignal && config_.stopSignal->load(std::memory_order_relaxed)) {
        stopped_ = true;
        return;
    }
    auto elapsed = std::chrono::steady_clock::now() - startTime_;
    if (elapsed >= config_.searchTime) {
        stopped_ = true;
    }
}

}  // namespace cchess
