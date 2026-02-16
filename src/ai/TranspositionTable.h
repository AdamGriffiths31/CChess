#ifndef CCHESS_TRANSPOSITION_TABLE_H
#define CCHESS_TRANSPOSITION_TABLE_H

#include "core/Move.h"

#include <cstdint>
#include <vector>

namespace cchess {

enum class TTBound : uint8_t { NONE = 0, EXACT = 1, LOWER = 2, UPPER = 3 };

// 16-byte packed TT entry.
// genBound packs generation (upper 6 bits) + bound (lower 2 bits).
// hashVerify stores upper 16 bits of the Zobrist hash; lower bits are used for indexing.
struct TTEntry {
    int32_t score = 0;
    uint16_t hashVerify = 0;
    int16_t depth = 0;
    uint8_t genBound = 0;  // generation(6) | bound(2)
    Move bestMove;         // 4 bytes

    TTBound bound() const { return static_cast<TTBound>(genBound & 0x3); }
    uint8_t generation() const { return genBound >> 2; }
    bool isEmpty() const { return (genBound & 0x3) == 0; }
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
    explicit TranspositionTable(size_t sizeMB = 128);

    bool probe(uint64_t hash, TTEntry& entry);
    void store(uint64_t hash, int score, int depth, TTBound bound, Move bestMove);
    void newSearch();
    void clear();

    size_t entryCount() const { return mask_ + 1; }
    size_t usedEntries() const;
    double occupancy() const {
        return 100.0 * static_cast<double>(usedEntries()) / static_cast<double>(entryCount());
    }

    TTStats& stats() { return stats_; }
    const TTStats& stats() const { return stats_; }

private:
    // Lower bits of hash select the index; upper 16 bits stored for verification.
    size_t index(uint64_t hash) const { return static_cast<size_t>(hash) & mask_; }
    static uint16_t verifyKey(uint64_t hash) { return static_cast<uint16_t>(hash >> 48); }

    std::vector<TTEntry> entries_;
    size_t mask_ = 0;
    uint8_t generation_ = 0;  // 6-bit, wraps at 64
    TTStats stats_;
};

}  // namespace cchess

#endif  // CCHESS_TRANSPOSITION_TABLE_H
