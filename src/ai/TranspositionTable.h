#ifndef CCHESS_TRANSPOSITION_TABLE_H
#define CCHESS_TRANSPOSITION_TABLE_H

#include "core/Move.h"

#include <cstdint>
#include <vector>

namespace cchess {

enum class TTBound : uint8_t { NONE = 0, EXACT, LOWER, UPPER };

struct TTEntry {
    uint64_t hashVerify = 0;
    int16_t score = 0;
    int16_t depth = 0;
    TTBound bound = TTBound::NONE;
    Move bestMove;
};

struct TTStats {
    uint64_t probes = 0;
    uint64_t hits = 0;
    uint64_t cutoffs = 0;  // hit with sufficient depth
    uint64_t stores = 0;
    uint64_t overwrites = 0;  // replaced a non-empty slot

    void reset() { probes = hits = cutoffs = stores = overwrites = 0; }
    double hitRate() const {
        return probes ? 100.0 * static_cast<double>(hits) / static_cast<double>(probes) : 0.0;
    }
    double cutoffRate() const {
        return probes ? 100.0 * static_cast<double>(cutoffs) / static_cast<double>(probes) : 0.0;
    }
};

// Mate scores are stored relative to root, not ply. Convert before storing
// and after probing so that mate-in-N is correct regardless of search path.
// The threshold (SCORE_MATE - 200) must match the mate score in Eval.h.
constexpr int TT_MATE_THRESHOLD = 100000 - 200;

inline int scoreToTT(int score, int ply) {
    if (score >= TT_MATE_THRESHOLD)
        return score + ply;
    if (score <= -TT_MATE_THRESHOLD)
        return score - ply;
    return score;
}

inline int scoreFromTT(int score, int ply) {
    if (score >= TT_MATE_THRESHOLD)
        return score - ply;
    if (score <= -TT_MATE_THRESHOLD)
        return score + ply;
    return score;
}

class TranspositionTable {
public:
    explicit TranspositionTable(size_t sizeMB = 16);

    bool probe(uint64_t hash, TTEntry& entry);
    void store(uint64_t hash, int score, int depth, TTBound bound, Move bestMove);
    void clear();

    size_t entryCount() const { return entries_.size(); }
    size_t usedEntries() const;
    double occupancy() const {
        return 100.0 * static_cast<double>(usedEntries()) / static_cast<double>(entries_.size());
    }

    TTStats& stats() { return stats_; }
    const TTStats& stats() const { return stats_; }

private:
    size_t index(uint64_t hash) const { return hash % entries_.size(); }

    std::vector<TTEntry> entries_;
    TTStats stats_;
};

}  // namespace cchess

#endif  // CCHESS_TRANSPOSITION_TABLE_H
