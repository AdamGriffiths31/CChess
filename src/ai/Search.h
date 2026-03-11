#ifndef CCHESS_SEARCH_H
#define CCHESS_SEARCH_H

#include "ai/PawnTable.h"
#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Board.h"
#include "core/Move.h"
#include "core/MoveList.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace cchess {

constexpr int MAX_PLY = 128;

struct SearchInfo {
    int depth = 0;
    int score = 0;
    uint64_t nodes = 0;
    int timeMs = 0;
    std::vector<Move> pv;
};

using InfoCallback = std::function<void(const SearchInfo&)>;

class Search {
public:
    Search(const Board& board, const SearchConfig& config, TranspositionTable& tt,
           eval::PawnTable& pt, InfoCallback infoCallback = nullptr,
           std::vector<uint64_t> gameHistory = {});

    Move findBestMove();

    uint64_t totalNodes() const { return nodes_; }

private:
    int negamax(int depth, int alpha, int beta, int ply, bool inCheck, bool nullOk = true);
    int quiescence(int alpha, int beta, int ply);
    void checkTime();
    std::vector<Move> extractPV(int maxLength);

    bool isRepetition() const;

    Board board_;
    SearchConfig config_;
    TranspositionTable& tt_;
    eval::PawnTable& pawnTable_;
    InfoCallback infoCallback_;

    // Positions are stored in two separate containers because the draw threshold differs:
    // any repetition within the current search path is treated as an immediate draw
    // (preventing the engine from chasing a repetition it could avoid), while game
    // history requires the standard three-fold repetition before scoring as a draw.
    std::vector<uint64_t> gameHistory_;
    std::vector<uint64_t> searchStack_;

    std::chrono::steady_clock::time_point startTime_;
    bool stopped_;
    uint64_t nodes_;

    // Two quiet moves per ply that recently caused a beta-cutoff.
    // Tried early in sibling nodes — a refutation at one branch often works in
    // similar positions at the same search depth, improving move ordering without
    // the cost of MVV-LVA or history lookups.
    std::array<std::array<Move, 2>, MAX_PLY> killers_;
    void storeKiller(int ply, const Move& move);

    // Butterfly history scores for quiet moves indexed by [color][from][to].
    // Moves that historically cause cutoffs are ordered earlier, improving move
    // ordering for quiet positions where MVV-LVA doesn't apply.
    static constexpr int MAX_HISTORY = 8192;
    std::array<std::array<std::array<int, 64>, 64>, 2> history_{};
    void updateHistory(int colorIdx, int from, int to, int bonus);

    // Precomputed reductions for late-move searching, indexed by [depth][moveIndex].
    // Since move ordering puts the best candidates first, later moves are likely
    // weaker and can be searched at reduced depth without missing good moves —
    // a re-search is triggered only if the reduced result beats alpha.
    static constexpr int MAX_LMR_DEPTH = 64;
    static constexpr int MAX_LMR_MOVES = 64;
    static std::array<std::array<int, MAX_LMR_MOVES>, MAX_LMR_DEPTH> lmrTable_;
    static bool lmrInitialized_;
    static void initLMR();
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_H
