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
           InfoCallback infoCallback = nullptr, std::vector<uint64_t> gameHistory = {});

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
    InfoCallback infoCallback_;

    // Repetition detection: game history before search + current search path
    std::vector<uint64_t> gameHistory_;  // hashes from game moves before search
    std::vector<uint64_t> searchStack_;  // hashes pushed during search (size == ply)

    std::chrono::steady_clock::time_point startTime_;
    bool stopped_;
    uint64_t nodes_;

    // Killer moves: two quiet moves per ply that caused a beta-cutoff
    std::array<std::array<Move, 2>, MAX_PLY> killers_;
    void storeKiller(int ply, const Move& move);

    // Late Move Reduction table: lmrTable_[depth][moveIndex]
    static constexpr int MAX_LMR_DEPTH = 64;
    static constexpr int MAX_LMR_MOVES = 64;
    static std::array<std::array<int, MAX_LMR_MOVES>, MAX_LMR_DEPTH> lmrTable_;
    static bool lmrInitialized_;
    static void initLMR();
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_H
