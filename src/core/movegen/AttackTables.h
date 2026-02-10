#ifndef CCHESS_ATTACK_TABLES_H
#define CCHESS_ATTACK_TABLES_H

#include "../Bitboard.h"
#include "../Square.h"

namespace cchess {

extern Bitboard KNIGHT_ATTACKS[64];
extern Bitboard KING_ATTACKS[64];

Bitboard rookAttacks(Square sq, Bitboard occupied);
Bitboard bishopAttacks(Square sq, Bitboard occupied);

}  // namespace cchess

#endif  // CCHESS_ATTACK_TABLES_H
