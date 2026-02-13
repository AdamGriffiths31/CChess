#include "Zobrist.h"

namespace cchess {
namespace zobrist {

uint64_t pieceKeys[2][6][64];
uint64_t sideKey;
uint64_t castlingKeys[16];
uint64_t enPassantKeys[8];

static uint64_t xorshift64(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

void init() {
    uint64_t seed = 0x3A9F1C7D5E8B4026ULL;  // fixed seed for reproducibility

    for (int c = 0; c < 2; ++c)
        for (int pt = 0; pt < 6; ++pt)
            for (int sq = 0; sq < 64; ++sq)
                pieceKeys[c][pt][sq] = xorshift64(seed);

    sideKey = xorshift64(seed);

    for (int i = 0; i < 16; ++i)
        castlingKeys[i] = xorshift64(seed);

    for (int f = 0; f < 8; ++f)
        enPassantKeys[f] = xorshift64(seed);
}

}  // namespace zobrist
}  // namespace cchess
