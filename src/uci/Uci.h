#ifndef CCHESS_UCI_H
#define CCHESS_UCI_H

#include "ai/TranspositionTable.h"
#include "core/Board.h"

#include <atomic>
#include <cstdint>
#include <sstream>
#include <thread>
#include <vector>

namespace cchess {

class Uci {
public:
    void loop();

private:
    void handleUci();
    void handleIsReady();
    void handleNewGame();
    void handlePosition(std::istringstream& args);
    void handleGo(std::istringstream& args);
    void handleStop();

    void joinSearch();

    Board board_;
    std::vector<uint64_t> gameHistory_;
    TranspositionTable tt_;
    std::atomic<bool> stopFlag_{false};
    std::thread searchThread_;
};

}  // namespace cchess

#endif  // CCHESS_UCI_H
