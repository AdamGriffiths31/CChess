#ifndef CCHESS_SEARCH_H
#define CCHESS_SEARCH_H

#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Board.h"
#include "core/Move.h"
#include "core/MoveList.h"

#include <chrono>
#include <cstdint>

namespace cchess {

class Search {
public:
    Search(const Board& board, const SearchConfig& config, TranspositionTable& tt);

    Move findBestMove();

private:
    int negamax(int depth, int alpha, int beta, int ply);
    int quiescence(int alpha, int beta, int ply);
    void checkTime();

    Board board_;
    SearchConfig config_;
    TranspositionTable& tt_;

    std::chrono::steady_clock::time_point startTime_;
    bool stopped_;
    uint64_t nodes_;
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_H
