#ifndef CCHESS_ENGINE_MATCH_H
#define CCHESS_ENGINE_MATCH_H

#include "core/Board.h"
#include "mode/OpponentList.h"

namespace cchess {

class EngineMatch {
public:
    // timeMs: base time in milliseconds, incMs: increment per move in milliseconds
    explicit EngineMatch(const Opponent& opponent, int timeMs = 180000, int incMs = 2000);

    void play();

private:
    bool applyUciMove(const std::string& uciMove);
    int allocateTime(int remainingMs, int incMs) const;

    Opponent opponent_;
    int timeMs_;
    int incMs_;
    Board board_;
};

}  // namespace cchess

#endif  // CCHESS_ENGINE_MATCH_H
