#ifndef CCHESS_ATTACK_TABLES_H
#define CCHESS_ATTACK_TABLES_H

#include "../Bitboard.h"
#include "../Square.h"

#include <cstdint>

namespace cchess {

extern Bitboard KNIGHT_ATTACKS[64];
extern Bitboard KING_ATTACKS[64];

// Exposed for inline lookups
extern Bitboard ROOK_MASKS[64];
extern Bitboard BISHOP_MASKS[64];
extern uint64_t ROOK_MAGICS[64];
extern uint64_t BISHOP_MAGICS[64];
extern int ROOK_SHIFTS[64];
extern int BISHOP_SHIFTS[64];
extern Bitboard ROOK_TABLE[64][4096];
extern Bitboard BISHOP_TABLE[64][512];

inline Bitboard rookAttacks(Square sq, Bitboard occupied) {
    return ROOK_TABLE[sq][((occupied & ROOK_MASKS[sq]) * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq]];
}

inline Bitboard bishopAttacks(Square sq, Bitboard occupied) {
    return BISHOP_TABLE[sq]
                       [((occupied & BISHOP_MASKS[sq]) * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq]];
}

}  // namespace cchess

#endif  // CCHESS_ATTACK_TABLES_H
