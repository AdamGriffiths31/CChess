#include "ai/TranspositionTable.h"

#include <algorithm>
#include <cassert>
#include <climits>

namespace cchess {

TranspositionTable::TranspositionTable(size_t sizeMB) {
    size_t numEntries = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
    if (numEntries == 0)
        numEntries = 1;
    entries_.resize(numEntries);
}

bool TranspositionTable::probe(uint64_t hash, TTEntry& entry) {
    ++stats_.probes;
    const TTEntry& slot = entries_[index(hash)];
    if (slot.bound != TTBound::NONE && slot.hashVerify == hash) {
        entry = slot;
        ++stats_.hits;
        return true;
    }
    return false;
}

void TranspositionTable::store(uint64_t hash, int score, int depth, TTBound bound, Move bestMove) {
    assert(bound != TTBound::NONE);
    assert(depth >= 0);
    ++stats_.stores;
    TTEntry& slot = entries_[index(hash)];
    if (slot.bound != TTBound::NONE && slot.hashVerify == hash && depth < slot.depth)
        return;
    if (slot.bound != TTBound::NONE)
        ++stats_.overwrites;
    slot.hashVerify = hash;
    slot.score = score;
    slot.depth = static_cast<int16_t>(depth);
    slot.bound = bound;
    slot.bestMove = bestMove;
}

void TranspositionTable::clear() {
    std::fill(entries_.begin(), entries_.end(), TTEntry{});
    stats_.reset();
}

size_t TranspositionTable::usedEntries() const {
    return static_cast<size_t>(
        std::count_if(entries_.begin(), entries_.end(),
                      [](const TTEntry& e) { return e.bound != TTBound::NONE; }));
}

}  // namespace cchess
