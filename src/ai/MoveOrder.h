#ifndef CCHESS_MOVE_ORDER_H
#define CCHESS_MOVE_ORDER_H

#include "core/Move.h"
#include "core/MoveList.h"
#include "core/Position.h"

namespace cchess {

// ============================================================================
// Move ordering features:
//   - MVV-LVA (Most Valuable Victim - Least Valuable Attacker) for captures
//   - Promotion bonus
// ============================================================================

class MoveOrder {
public:
    // Score a single move for ordering (higher = try first)
    static int score(const Move& move, const Position& pos);

    // Sort a full move list (captures/promotions first, then quiets)
    static void sort(MoveList& moves, const Position& pos);

    // Sort with TT move prioritized first
    static void sort(MoveList& moves, const Position& pos, const Move& ttMove);

    // Extract and sort only captures + promotions from a move list.
    // Returns the count written into `out`.
    static size_t extractCaptures(const MoveList& moves, const Position& pos, Move* out,
                                  size_t maxOut);
};

}  // namespace cchess

#endif  // CCHESS_MOVE_ORDER_H
