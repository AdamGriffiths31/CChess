#include "ai/TranspositionTable.h"

#include <algorithm>
#include <cassert>
#include <climits>

namespace cchess {

TranspositionTable::TranspositionTable(size_t sizeMB) {
    size_t numEntries = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
    // Round down to power of 2
    size_t pot = 1;
    while (pot * 2 <= numEntries)
        pot *= 2;
    entries_.resize(pot);
    mask_ = pot - 1;
}

bool TranspositionTable::probe(uint64_t hash, TTEntry& entry) {
    ++stats_.probes;
    const TTEntry& slot = entries_[index(hash)];
    if (!slot.isEmpty() && slot.hashVerify == verifyKey(hash)) {
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

    uint16_t key16 = verifyKey(hash);
    bool occupied = !slot.isEmpty();
    bool samePosition = occupied && slot.hashVerify == key16;
    bool stale = occupied && slot.generation() != generation_;

    // Same position at deeper depth — keep the existing entry
    if (samePosition && depth < slot.depth)
        return;

    // Different position: always replace stale entries.
    // For fresh entries, only replace if new depth >= old depth or new bound is EXACT.
    if (occupied && !samePosition && !stale) {
        if (depth < slot.depth && bound != TTBound::EXACT)
            return;
    }

    if (occupied)
        ++stats_.overwrites;
    slot.hashVerify = key16;
    slot.score = score;
    slot.depth = static_cast<int16_t>(depth);
    slot.genBound = static_cast<uint8_t>((generation_ << 2) | static_cast<uint8_t>(bound));
    slot.bestMove = bestMove;
}

void TranspositionTable::newSearch() {
    generation_ = (generation_ + 1) & 0x3F;  // wrap at 64 (6 bits)
}

void TranspositionTable::clear() {
    std::fill(entries_.begin(), entries_.end(), TTEntry{});
    generation_ = 0;
    stats_.reset();
}

size_t TranspositionTable::usedEntries() const {
    return static_cast<size_t>(std::count_if(entries_.begin(), entries_.end(),
                                             [](const TTEntry& e) { return !e.isEmpty(); }));
}

}  // namespace cchess
