#ifndef CCHESS_BITBOARD_H
#define CCHESS_BITBOARD_H

#include "Types.h"

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(__popcnt64)
#endif

namespace cchess {

using Bitboard = uint64_t;

// ------- Constants -------

constexpr Bitboard BB_EMPTY = 0ULL;
constexpr Bitboard BB_ALL = ~0ULL;

// File bitboards
constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_B_BB = FILE_A_BB << 1;
constexpr Bitboard FILE_C_BB = FILE_A_BB << 2;
constexpr Bitboard FILE_D_BB = FILE_A_BB << 3;
constexpr Bitboard FILE_E_BB = FILE_A_BB << 4;
constexpr Bitboard FILE_F_BB = FILE_A_BB << 5;
constexpr Bitboard FILE_G_BB = FILE_A_BB << 6;
constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;

// Rank bitboards
constexpr Bitboard RANK_1_BB = 0xFFULL;
constexpr Bitboard RANK_2_BB = RANK_1_BB << 8;
constexpr Bitboard RANK_3_BB = RANK_1_BB << 16;
constexpr Bitboard RANK_4_BB = RANK_1_BB << 24;
constexpr Bitboard RANK_5_BB = RANK_1_BB << 32;
constexpr Bitboard RANK_6_BB = RANK_1_BB << 40;
constexpr Bitboard RANK_7_BB = RANK_1_BB << 48;
constexpr Bitboard RANK_8_BB = RANK_1_BB << 56;

// Lookup arrays for file/rank by index
constexpr Bitboard FILE_BB[8] = {FILE_A_BB, FILE_B_BB, FILE_C_BB, FILE_D_BB,
                                 FILE_E_BB, FILE_F_BB, FILE_G_BB, FILE_H_BB};

constexpr Bitboard RANK_BB[8] = {RANK_1_BB, RANK_2_BB, RANK_3_BB, RANK_4_BB,
                                 RANK_5_BB, RANK_6_BB, RANK_7_BB, RANK_8_BB};

// ------- Bit Manipulation -------

// Population count (number of set bits)
inline int popCount(Bitboard b) {
#ifdef _MSC_VER
    return static_cast<int>(__popcnt64(b));
#else
    return __builtin_popcountll(b);
#endif
}

// Least significant bit index (undefined if b == 0)
inline Square lsb(Bitboard b) {
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return static_cast<Square>(idx);
#else
    return static_cast<Square>(__builtin_ctzll(b));
#endif
}

// Most significant bit index (undefined if b == 0)
inline Square msb(Bitboard b) {
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return static_cast<Square>(idx);
#else
    return static_cast<Square>(63 ^ __builtin_clzll(b));
#endif
}

// Extract and clear least significant bit, return its index
inline Square popLsb(Bitboard& b) {
    Square sq = lsb(b);
    b &= b - 1;
    return sq;
}

// Check if more than one bit is set
inline bool moreThanOne(Bitboard b) {
    return b & (b - 1);
}

// ------- Square-Bitboard Conversions -------

// Create a bitboard with a single bit set at the given square
inline Bitboard squareBB(Square sq) {
    return 1ULL << sq;
}

// Test if a specific bit is set
inline bool testBit(Bitboard b, Square sq) {
    return (b & squareBB(sq)) != 0;
}

// Set a bit
inline Bitboard& setBit(Bitboard& b, Square sq) {
    return b |= squareBB(sq);
}

// Clear a bit
inline Bitboard& clearBit(Bitboard& b, Square sq) {
    return b &= ~squareBB(sq);
}

// ------- Shift Operations -------

// Shift north (toward rank 8)
inline Bitboard shiftNorth(Bitboard b) {
    return b << 8;
}

// Shift south (toward rank 1)
inline Bitboard shiftSouth(Bitboard b) {
    return b >> 8;
}

// Shift east (toward file H), masking out file A wraps
inline Bitboard shiftEast(Bitboard b) {
    return (b & ~FILE_H_BB) << 1;
}

// Shift west (toward file A), masking out file H wraps
inline Bitboard shiftWest(Bitboard b) {
    return (b & ~FILE_A_BB) >> 1;
}

// Diagonal shifts
inline Bitboard shiftNorthEast(Bitboard b) {
    return (b & ~FILE_H_BB) << 9;
}
inline Bitboard shiftNorthWest(Bitboard b) {
    return (b & ~FILE_A_BB) << 7;
}
inline Bitboard shiftSouthEast(Bitboard b) {
    return (b & ~FILE_H_BB) >> 7;
}
inline Bitboard shiftSouthWest(Bitboard b) {
    return (b & ~FILE_A_BB) >> 9;
}

// ------- File/Rank Bitboard Helpers -------

// Get the bitboard for a specific file
inline Bitboard fileBB(File f) {
    return FILE_BB[f];
}

// Get the bitboard for a specific rank
inline Bitboard rankBB(Rank r) {
    return RANK_BB[r];
}

}  // namespace cchess

#endif  // CCHESS_BITBOARD_H
