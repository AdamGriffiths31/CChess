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
// Construction
// ============================================================================

Search::Search(const Board& board, const SearchConfig& config, TranspositionTable& tt,
               InfoCallback infoCallback, std::vector<uint64_t> gameHistory)
    : board_(board),
      config_(config),
      tt_(tt),
      infoCallback_(std::move(infoCallback)),
      gameHistory_(std::move(gameHistory)),
      stopped_(false),
      nodes_(0) {
    initLMR();
}

// Store a killer move at the given ply: shift existing killer to slot 1, new move to slot 0.
// Only store quiet moves (not captures or promotions).
void Search::storeKiller(int ply, const Move& move) {
    assert(ply >= 0 && ply < MAX_PLY);
    auto p = static_cast<size_t>(ply);
    // Avoid duplicate entries
    if (killers_[p][0] == move)
        return;
    killers_[p][1] = killers_[p][0];
    killers_[p][0] = move;
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
// Repetition detection
// ============================================================================

// Returns true if the current position is a draw by repetition.
//
// Strategy: treat any repetition as an immediate draw during search.
// - One prior occurrence within the current search path (searchStack_) = draw.
//   This avoids the engine voluntarily entering a line it can repeat indefinitely.
// - Two prior occurrences in the game history (gameHistory_) = draw.
//   This correctly implements 3-fold repetition for positions reached before search.
//
// We only look back as far as the halfmove clock allows, since any pawn move
// or capture resets it and makes repetition impossible beyond that point.
bool Search::isRepetition() const {
    uint64_t hash = board_.position().hash();
    int halfmove = board_.position().halfmoveClock();

    // Count occurrences in current search path (excluding the current node itself).
    // Any match here = 2-fold repetition within the search = treat as draw.
    int searchSize = static_cast<int>(searchStack_.size());
    int searchLookback = std::min(searchSize, halfmove);
    for (int i = searchSize - 1; i >= searchSize - searchLookback; --i) {
        if (searchStack_[static_cast<size_t>(i)] == hash)
            return true;
    }

    // Count occurrences in game history (positions before search started).
    // Need two matches there for 3-fold repetition.
    int historyLookback = halfmove - searchSize;
    if (historyLookback > 0) {
        int histSize = static_cast<int>(gameHistory_.size());
        int start = histSize - historyLookback;
        if (start < 0)
            start = 0;
        int historyCount = 0;
        for (int i = start; i < histSize; ++i) {
            if (gameHistory_[static_cast<size_t>(i)] == hash) {
                if (++historyCount >= 2)
                    return true;
            }
        }
    }

    return false;
}

// ============================================================================
// Search
// ============================================================================

// Runs iterative deepening and returns the best move found within the time/
// depth limits.  Each iteration calls negamax with the following features:
//   - Negamax + alpha-beta pruning
//   - Principal Variation Search (PVS)
//   - Null Move Pruning (NMP)
//   - Late Move Reductions (LMR)
//   - Quiescence search (captures + promotions only)
//   - Transposition table with best-move ordering
//   - MVV-LVA capture ordering + killer move heuristic (see MoveOrder.cpp)
//   - Repetition detection (2-fold within search, 3-fold from game history)
Move Search::findBestMove() {
    startTime_ = std::chrono::steady_clock::now();
    stopped_ = false;
    nodes_ = 0;
    tt_.newSearch();
    for (auto& pair : killers_)
        pair[0] = pair[1] = Move{};
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

        uint64_t rootHash = board_.position().hash();
        for (size_t i = 0; i < moves.size(); ++i) {
            searchStack_.push_back(rootHash);
            UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
            ++nodes_;

            bool givesCheck = board_.isInCheck();

            int score;
            if (i == 0) {
                score = -negamax(depth - 1, -beta, -alpha, 1, givesCheck);
            } else {
                score = -negamax(depth - 1, -alpha - 1, -alpha, 1, givesCheck);
                if (score > alpha && score < beta)
                    score = -negamax(depth - 1, -beta, -alpha, 1, givesCheck);
            }

            board_.unmakeMove(moves[i], undo);
            searchStack_.pop_back();

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

int Search::negamax(int depth, int alpha, int beta, int ply, bool inCheck, bool nullOk) {
    assert(alpha < beta);
    assert(depth >= 0);
    assert(ply >= 0);

    if ((nodes_ & 1023) == 0)
        checkTime();
    if (stopped_)
        return 0;

    // Draw detection: 50-move rule and repetition
    if (board_.isDraw() || isRepetition())
        return eval::SCORE_DRAW;

    if (depth == 0)
        return quiescence(alpha, beta, ply);

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

    // Null move pruning: if we can skip our turn and still beat beta,
    // the position is so good that we can prune this branch.
    // Don't use when in check, at PV nodes, or after a previous null move.
    constexpr int NMP_REDUCTION = 2;
    bool isPvNode = (beta - alpha > 1);
    if (nullOk && !isPvNode && !inCheck && depth >= 3) {
        Square prevEp = board_.enPassantSquare();
        uint64_t prevHash = board_.position().hash();
        board_.makeNullMove();
        int nullScore =
            -negamax(depth - 1 - NMP_REDUCTION, -beta, -beta + 1, ply + 1, false, false);
        board_.unmakeNullMove(prevEp, prevHash);

        if (stopped_)
            return 0;
        if (nullScore >= beta)
            return beta;
    }

    MoveList moves = board_.getLegalMoves();

    if (moves.empty()) {
        if (inCheck) {
            return -(eval::SCORE_MATE - ply);  // Checkmate
        }
        return eval::SCORE_DRAW;  // Stalemate
    }

    assert(ply < MAX_PLY);
    const Move* killers = killers_[static_cast<size_t>(ply)].data();
    MoveOrder::sort(moves, board_.position(), ttMove, killers);

    int bestScore = -eval::SCORE_INFINITY;
    Move bestMoveInNode;
    int origAlpha = alpha;

    for (size_t i = 0; i < moves.size(); ++i) {
        // Push current hash so children can detect repetition back to this node
        searchStack_.push_back(posHash);
        UndoInfo undo = board_.makeMoveUnchecked(moves[i]);
        ++nodes_;

        bool givesCheck = board_.isInCheck();

        int score;
        if (i == 0) {
            score = -negamax(depth - 1, -beta, -alpha, ply + 1, givesCheck);
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

            score = -negamax(depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, givesCheck);

            // Re-search at full depth if reduced search beat alpha
            if (reduction > 0 && score > alpha)
                score = -negamax(depth - 1, -alpha - 1, -alpha, ply + 1, givesCheck);

            // PVS re-search with full window
            if (score > alpha && score < beta)
                score = -negamax(depth - 1, -beta, -alpha, ply + 1, givesCheck);
        }

        board_.unmakeMove(moves[i], undo);
        searchStack_.pop_back();

        if (stopped_)
            return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMoveInNode = moves[i];
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta) {
            // Store killer: quiet moves that cause beta-cutoffs
            if (!moves[i].isCapture() && !moves[i].isPromotion() && ply < MAX_PLY)
                storeKiller(ply, moves[i]);
            break;
        }
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
