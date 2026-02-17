#ifndef CCHESS_SEARCH_H
#define CCHESS_SEARCH_H

#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Board.h"
#include "core/Move.h"
#include "core/MoveList.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace cchess {

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
           InfoCallback infoCallback = nullptr);

    Move findBestMove();

    uint64_t totalNodes() const { return nodes_; }

private:
    int negamax(int depth, int alpha, int beta, int ply, bool inCheck, bool nullOk = true);
    int quiescence(int alpha, int beta, int ply);
    void checkTime();
    std::vector<Move> extractPV(int maxLength);

    Board board_;
    SearchConfig config_;
    TranspositionTable& tt_;
    InfoCallback infoCallback_;

    std::chrono::steady_clock::time_point startTime_;
    bool stopped_;
    uint64_t nodes_;

    // Late Move Reduction table: lmrTable_[depth][moveIndex]
    static constexpr int MAX_LMR_DEPTH = 64;
    static constexpr int MAX_LMR_MOVES = 64;
    static std::array<std::array<int, MAX_LMR_MOVES>, MAX_LMR_DEPTH> lmrTable_;
    static bool lmrInitialized_;
    static void initLMR();
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_H
