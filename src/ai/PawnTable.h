#ifndef CCHESS_PAWN_TABLE_H
#define CCHESS_PAWN_TABLE_H

#include "PST.h"

#include <algorithm>
#include <cstdint>

namespace cchess {
namespace eval {


struct PawnEntry {
    uint64_t key = 0;
    Score pawnScore;
    Score passedScore;
};


class PawnTable {
public:
    static constexpr int SIZE = 1 << 16;  // 65536 entries, must be power of 2
    static constexpr int MASK = SIZE - 1;

    PawnTable() { std::fill(entries_, entries_ + SIZE, PawnEntry{}); }

    PawnEntry* probe(uint64_t pawnKey) { return &entries_[pawnKey & MASK]; }

private:
    PawnEntry entries_[SIZE];
};

}  // namespace eval
}  // namespace cchess

#endif  // CCHESS_PAWN_TABLE_H
