#ifndef CCHESS_SEARCH_H
#define CCHESS_SEARCH_H

#include "ai/SearchConfig.h"
#include "core/Board.h"
#include "core/Move.h"
#include "core/MoveList.h"

#include <chrono>
#include <cstdint>

namespace cchess {

class Search {
public:
    Search(const Board& board, const SearchConfig& config);

    Move findBestMove();

private:
    int negamax(int depth, int alpha, int beta, int ply);
    void checkTime();
    void orderMoves(MoveList& moves);

    Board board_;
    SearchConfig config_;

    std::chrono::steady_clock::time_point startTime_;
    bool stopped_;
    uint64_t nodes_;
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_H
