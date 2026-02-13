#ifndef CCHESS_ZOBRIST_H
#define CCHESS_ZOBRIST_H

#include <cstdint>

namespace cchess {
namespace zobrist {

// Zobrist hash keys for incremental position hashing.
// Must call init() once at startup before any hashing.

extern uint64_t pieceKeys[2][6][64];  // [color][pieceType][square]
extern uint64_t sideKey;              // XORed when side to move is Black
extern uint64_t castlingKeys[16];     // indexed by CastlingRights (0-15)
extern uint64_t enPassantKeys[8];     // indexed by file (0-7)

void init();

}  // namespace zobrist
}  // namespace cchess

#endif  // CCHESS_ZOBRIST_H
