#include "ai/TranspositionTable.h"

#include <algorithm>
#include <cassert>
#include <climits>

namespace cchess {

TranspositionTable::TranspositionTable(size_t sizeMB) {
    size_t numClusters = (sizeMB * 1024 * 1024) / sizeof(TTCluster);
    // Round down to power of 2
    size_t pot = 1;
    while (pot * 2 <= numClusters)
        pot *= 2;
    clusters_.resize(pot);
    mask_ = pot - 1;
}

void TranspositionTable::store(uint64_t hash, int score, int depth, TTBound bound,
                               const Move& bestMove) {
    assert(bound != TTBound::NONE);
    assert(depth >= 0);
    ++stats_.stores;

    uint16_t key16 = verifyKey(hash);
    TTCluster& cluster = clusters_[clusterIndex(hash)];

    // Find the best slot to replace:
    // 1. Same position (update in place) — prefer this always
    // 2. Empty slot
    // 3. Oldest/shallowest entry (lowest depth - relative age score)
    TTEntry* replace = nullptr;
    int replaceScore = INT_MAX;

    for (int i = 0; i < 4; ++i) {
        TTEntry& e = cluster.entries[i];

        // Same position: update if new depth >= old, or new bound is EXACT
        if (!e.isEmpty() && e.hashVerify == key16) {
            if (depth < e.depth && bound != TTBound::EXACT)
                return;
            replace = &e;
            break;
        }

        // Empty slot — ideal
        if (e.isEmpty()) {
            replace = &e;
            break;
        }

        // Score: prefer replacing stale/shallow entries
        // Lower score = better replacement candidate
        int age = (generation_ - e.generation()) & 0x3F;
        int score_val = static_cast<int>(e.depth) - age * 4;
        if (score_val < replaceScore) {
            replaceScore = score_val;
            replace = &e;
        }
    }

    if (!replace)
        replace = &cluster.entries[0];

    if (!replace->isEmpty())
        ++stats_.overwrites;

    replace->hashVerify = key16;
    replace->score = static_cast<int16_t>(score);
    replace->depth = static_cast<int16_t>(depth);
    replace->genBound = static_cast<uint8_t>((generation_ << 2) | static_cast<uint8_t>(bound));
    replace->setMove(bestMove);
}

void TranspositionTable::newSearch() {
    generation_ = (generation_ + 1) & 0x3F;  // wrap at 64 (6 bits)
}

void TranspositionTable::clear() {
    std::fill(clusters_.begin(), clusters_.end(), TTCluster{});
    generation_ = 0;
    stats_.reset();
}

size_t TranspositionTable::usedEntries() const {
    size_t count = 0;
    for (const auto& cluster : clusters_)
        for (int i = 0; i < 4; ++i)
            if (!cluster.entries[i].isEmpty())
                ++count;
    return count;
}

}  // namespace cchess
