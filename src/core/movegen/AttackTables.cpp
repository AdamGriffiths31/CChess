#include "AttackTables.h"

#include <cstring>

namespace cchess {

Bitboard KNIGHT_ATTACKS[64];
Bitboard KING_ATTACKS[64];

// Direction offsets for sliding pieces (used during magic table init)
static const int ROOK_DIRECTIONS[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
static const int BISHOP_DIRECTIONS[][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

namespace {

// Magic bitboard tables
Bitboard ROOK_MASKS[64];
Bitboard BISHOP_MASKS[64];
uint64_t ROOK_MAGICS[64];
uint64_t BISHOP_MAGICS[64];
int ROOK_SHIFTS[64];
int BISHOP_SHIFTS[64];
Bitboard ROOK_TABLE[64][4096];    // 2MB — plain magics, max 12 relevant bits
Bitboard BISHOP_TABLE[64][512];   // 256KB — plain magics, max 9 relevant bits

// Compute the relevant occupancy mask for a rook on the given square.
// Excludes edge squares (blockers on edges don't affect further ray travel).
Bitboard computeRookMask(Square sq) {
    Bitboard mask = BB_EMPTY;
    int f = getFile(sq), r = getRank(sq);
    for (int i = r + 1; i <= 6; ++i) setBit(mask, makeSquare(static_cast<File>(f), static_cast<Rank>(i)));
    for (int i = r - 1; i >= 1; --i) setBit(mask, makeSquare(static_cast<File>(f), static_cast<Rank>(i)));
    for (int i = f + 1; i <= 6; ++i) setBit(mask, makeSquare(static_cast<File>(i), static_cast<Rank>(r)));
    for (int i = f - 1; i >= 1; --i) setBit(mask, makeSquare(static_cast<File>(i), static_cast<Rank>(r)));
    return mask;
}

// Compute the relevant occupancy mask for a bishop on the given square.
Bitboard computeBishopMask(Square sq) {
    Bitboard mask = BB_EMPTY;
    int f = getFile(sq), r = getRank(sq);
    for (int i = 1; f + i <= 6 && r + i <= 6; ++i) setBit(mask, makeSquare(static_cast<File>(f + i), static_cast<Rank>(r + i)));
    for (int i = 1; f + i <= 6 && r - i >= 1; ++i) setBit(mask, makeSquare(static_cast<File>(f + i), static_cast<Rank>(r - i)));
    for (int i = 1; f - i >= 1 && r + i <= 6; ++i) setBit(mask, makeSquare(static_cast<File>(f - i), static_cast<Rank>(r + i)));
    for (int i = 1; f - i >= 1 && r - i >= 1; ++i) setBit(mask, makeSquare(static_cast<File>(f - i), static_cast<Rank>(r - i)));
    return mask;
}

// Compute sliding attacks for a square with a given occupancy, used only during init.
// diagonal=true for bishop, false for rook.
Bitboard computeSlidingAttacks(Square sq, Bitboard occupied, bool diagonal) {
    Bitboard attacks = BB_EMPTY;
    const auto& dirs = diagonal ? BISHOP_DIRECTIONS : ROOK_DIRECTIONS;
    for (int d = 0; d < 4; ++d) {
        int df = dirs[d][0], dr = dirs[d][1];
        int f = getFile(sq) + df;
        int r = getRank(sq) + dr;
        while (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
            Square s = makeSquare(static_cast<File>(f), static_cast<Rank>(r));
            setBit(attacks, s);
            if (testBit(occupied, s)) break;
            f += df;
            r += dr;
        }
    }
    return attacks;
}

// Simple PRNG for magic number search
uint64_t randomState = 1070372ull;

uint64_t randomU64() {
    randomState ^= randomState >> 12;
    randomState ^= randomState << 25;
    randomState ^= randomState >> 27;
    return randomState * 2685821657736338717ull;
}

// Sparse random — AND three randoms for few bits set
uint64_t randomSparse() {
    return randomU64() & randomU64() & randomU64();
}

// Find a magic number for a given square/mask/bits, filling the attack table.
uint64_t findMagic(Square sq, Bitboard mask, int bits, Bitboard* table) {
    int tableSize = 1 << bits;

    // Enumerate all occupancy subsets and their corresponding attacks
    Bitboard occupancies[4096];
    Bitboard attacks[4096];
    int count = 0;

    Bitboard occ = 0;
    do {
        occupancies[count] = occ;
        attacks[count] = computeSlidingAttacks(sq, occ, bits <= 9);
        ++count;
        occ = (occ - mask) & mask;  // Carry-Rippler
    } while (occ);

    // Trial-and-error to find a collision-free magic
    for (int attempt = 0; attempt < 100000000; ++attempt) {
        uint64_t magic = randomSparse();

        // Quick reject: check that magic maps mask to enough high bits
        if (popCount((mask * magic) & 0xFF00000000000000ull) < 6) continue;

        std::memset(table, 0, sizeof(Bitboard) * static_cast<size_t>(tableSize));
        bool ok = true;

        for (int i = 0; i < count; ++i) {
            int idx = static_cast<int>((occupancies[i] * magic) >> (64 - bits));
            if (table[idx] == BB_EMPTY) {
                table[idx] = attacks[i];
            } else if (table[idx] != attacks[i]) {
                ok = false;
                break;
            }
        }

        if (ok) return magic;
    }

    // Should never reach here with reasonable bit counts
    return 0;
}

bool initAttackTables() {
    static const int knightOffsets[][2] = {{2, 1},  {2, -1},  {-2, 1}, {-2, -1},
                                           {1, 2},  {1, -2},  {-1, 2}, {-1, -2}};
    static const int kingOffsets[][2] = {{1, 0}, {-1, 0}, {0, 1},  {0, -1},
                                         {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (Square sq = 0; sq < 64; ++sq) {
        int f = getFile(sq);
        int r = getRank(sq);

        Bitboard bb = BB_EMPTY;
        for (const auto* off : knightOffsets) {
            int nf = f + off[0], nr = r + off[1];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                setBit(bb, makeSquare(static_cast<File>(nf), static_cast<Rank>(nr)));
        }
        KNIGHT_ATTACKS[sq] = bb;

        bb = BB_EMPTY;
        for (const auto* off : kingOffsets) {
            int nf = f + off[0], nr = r + off[1];
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7)
                setBit(bb, makeSquare(static_cast<File>(nf), static_cast<Rank>(nr)));
        }
        KING_ATTACKS[sq] = bb;
    }

    // Initialize magic bitboard tables
    for (Square sq = 0; sq < 64; ++sq) {
        // Rook
        ROOK_MASKS[sq] = computeRookMask(sq);
        int rookBits = popCount(ROOK_MASKS[sq]);
        ROOK_SHIFTS[sq] = 64 - rookBits;
        ROOK_MAGICS[sq] = findMagic(sq, ROOK_MASKS[sq], rookBits, ROOK_TABLE[sq]);

        // Bishop
        BISHOP_MASKS[sq] = computeBishopMask(sq);
        int bishopBits = popCount(BISHOP_MASKS[sq]);
        BISHOP_SHIFTS[sq] = 64 - bishopBits;
        BISHOP_MAGICS[sq] = findMagic(sq, BISHOP_MASKS[sq], bishopBits, BISHOP_TABLE[sq]);
    }

    return true;
}

bool tablesInitialized = initAttackTables();

}  // anonymous namespace

// O(1) sliding attack lookups
Bitboard rookAttacks(Square sq, Bitboard occupied) {
    return ROOK_TABLE[sq][((occupied & ROOK_MASKS[sq]) * ROOK_MAGICS[sq]) >> ROOK_SHIFTS[sq]];
}

Bitboard bishopAttacks(Square sq, Bitboard occupied) {
    return BISHOP_TABLE[sq][((occupied & BISHOP_MASKS[sq]) * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq]];
}

}  // namespace cchess
