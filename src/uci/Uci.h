#ifndef CCHESS_UCI_H
#define CCHESS_UCI_H

#include "ai/TranspositionTable.h"
#include "core/Board.h"

#include <atomic>
#include <sstream>
#include <thread>

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
    TranspositionTable tt_{16};
    std::atomic<bool> stopFlag_{false};
    std::thread searchThread_;
};

}  // namespace cchess

#endif  // CCHESS_UCI_H
