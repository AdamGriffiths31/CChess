#ifndef CCHESS_PERFTRUNNER_H
#define CCHESS_PERFTRUNNER_H

#include "../core/Board.h"

#include <cstdint>
#include <string>

namespace cchess {

class PerftRunner {
public:
    // Run the interactive perft session
    static void run();

private:
    // Core perft function: count leaf nodes at given depth
    static uint64_t perft(Board& board, int depth);

    // Divide: show per-move node counts at root
    static void divide(Board& board, int depth);
};

}  // namespace cchess

#endif  // CCHESS_PERFTRUNNER_H
