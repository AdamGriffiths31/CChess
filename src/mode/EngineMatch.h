#ifndef CCHESS_ENGINE_MATCH_H
#define CCHESS_ENGINE_MATCH_H

#include "ai/TranspositionTable.h"
#include "core/Board.h"
#include "core/Move.h"
#include "core/Types.h"
#include "mode/OpponentList.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cchess {

struct MoveRecord {
    int moveNumber = 0;
    std::string san;
    std::string uci;
    Color side = Color::White;
    int depthReached = 0;
    int score = 0;  // centipawns, white-relative
    uint64_t nodes = 0;
    int timeMs = 0;
    uint64_t nps = 0;
    std::vector<Move> pv;
    bool hasCChessInfo = false;  // true for CChess moves, false for opponent
};

struct GameSummary {
    uint64_t totalNodes = 0;
    int totalDepth = 0;
    uint64_t totalNps = 0;
    int cchessMoves = 0;
    int totalTimeMs = 0;

    static GameSummary fromLog(const std::vector<MoveRecord>& log) {
        GameSummary s;
        for (const auto& rec : log) {
            if (rec.hasCChessInfo) {
                s.totalNodes += rec.nodes;
                s.totalDepth += rec.depthReached;
                s.totalNps += rec.nps;
                s.totalTimeMs += rec.timeMs;
                ++s.cchessMoves;
            }
        }
        return s;
    }
};

class EngineMatch {
public:
    // timeMs: base time in milliseconds, incMs: increment per move in milliseconds
    explicit EngineMatch(const Opponent& opponent, int timeMs = 180000, int incMs = 2000);

    void play();

private:
    int allocateTime(int remainingMs, int incMs) const;
    void writeGameReport(const std::string& result, const TTStats& ttStats,
                         double ttOccupancy) const;

    Opponent opponent_;
    int timeMs_;
    int incMs_;
    Board board_;
    std::vector<MoveRecord> moveLog_;
};

}  // namespace cchess

#endif  // CCHESS_ENGINE_MATCH_H
