#include "ai/Eval.h"

#include "core/Bitboard.h"
#include "core/movegen/AttackTables.h"

#include <algorithm>

namespace cchess {
namespace eval {

// Material values (PeSTO-derived)
constexpr Score MATERIAL_VALUE[] = {
    S(82, 94),     // Pawn
    S(337, 281),   // Knight
    S(365, 297),   // Bishop
    S(477, 512),   // Rook
    S(1025, 936),  // Queen
    S(0, 0),       // King
};

// PeSTO piece-square tables (PST-only, material NOT included).
// From White's perspective (a1 = index 0). For Black, mirror via sq ^ 56.

// clang-format off
constexpr Score PAWN_PST[64] = {
    S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),  // rank 1
    S(-35,  13), S( -1,   8), S(-20,   8), S(-23, -11), S(-15,  -1), S( 24,  -2), S( 38,   6), S(-22,  -7),  // rank 2
    S(-26,   4), S( -4,   7), S( -4,  -6), S(-10,   1), S(  3,   0), S(  3,  -5), S( 33,  -1), S(-12,  -8),  // rank 3
    S(-27,  13), S( -2,   9), S( -5,  -3), S( 12,  -7), S( 17,  -7), S(  6,  -8), S( 10,   3), S(-25,  -1),  // rank 4
    S(-14,  32), S( 13,  24), S(  6,  13), S( 21,   5), S( 23,  -2), S( 12,   4), S( 17,  17), S(-23,  17),  // rank 5
    S( -6,  94), S(  7, 100), S( 26,  85), S( 31,  67), S( 65,  56), S( 56,  53), S( 25,  82), S(-20,  84),  // rank 6
    S( 98, 178), S(134, 173), S( 61, 158), S( 95, 134), S( 68, 147), S(126, 132), S( 34, 165), S(-11, 187),  // rank 7
    S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),  // rank 8
};

constexpr Score KNIGHT_PST[64] = {
    S(-105, -29), S(-21, -51), S(-58, -23), S(-33, -15), S(-17, -22), S(-28, -18), S(-19, -50), S(-23, -64),  // rank 1
    S(-29,  -42), S(-53, -20), S(-12, -10), S( -3,  -5), S( -1,  -2), S( 18, -20), S(-14, -23), S(-19, -44),  // rank 2
    S(-23,  -23), S( -9,  -3), S( 12,  -1), S( 10,  15), S( 19,  10), S( 17,  -3), S( 25, -20), S(-16, -22),  // rank 3
    S(-13,  -18), S(  4,  -6), S( 16,  16), S( 13,  25), S( 28,  16), S( 19,  17), S( 21,   4), S( -8, -18),  // rank 4
    S( -9,  -17), S( 17,   3), S( 19,  22), S( 53,  22), S( 37,  22), S( 69,  11), S( 18,   8), S( 22, -18),  // rank 5
    S(-47,  -24), S( 60, -20), S( 37,  10), S( 65,   9), S( 84,  -1), S(129,  -9), S( 73, -19), S( 44, -41),  // rank 6
    S(-73,  -25), S(-41,  -8), S( 72, -25), S( 36,  -2), S( 23,  -9), S( 62, -25), S(  7, -24), S(-17, -52),  // rank 7
    S(-167, -58), S(-89, -38), S(-34, -13), S(-49, -28), S( 61, -31), S(-97, -27), S(-15, -63), S(-107,-99),  // rank 8
};

constexpr Score BISHOP_PST[64] = {
    S(-33, -23), S( -3,  -9), S(-14, -23), S(-21,  -5), S(-13,  -9), S(-12, -16), S(-39,  -5), S(-21, -17),  // rank 1
    S(  4, -14), S( 15, -18), S( 16,  -7), S(  0,  -1), S(  7,   4), S( 21,  -9), S( 33, -15), S(  1, -27),  // rank 2
    S(  0, -12), S( 15,  -3), S( 15,   8), S( 15,  10), S( 14,  13), S( 27,   3), S( 18,  -7), S( 10, -15),  // rank 3
    S( -6,  -6), S( 13,   3), S( 13,  13), S( 26,  19), S( 34,   7), S( 12,  10), S( 10,  -3), S(  4,  -9),  // rank 4
    S( -4,  -3), S(  5,   9), S( 19,  12), S( 50,   9), S( 37,  14), S( 37,  10), S(  7,   3), S( -2,   2),  // rank 5
    S(-16,   2), S( 37,  -8), S( 43,   0), S( 40,  -1), S( 35,  -2), S( 50,   6), S( 37,   0), S( -2,   4),  // rank 6
    S(-26,  -8), S( 16,  -4), S(-18,   7), S(-13, -12), S( 30,  -3), S( 59, -13), S( 18,  -4), S(-47, -14),  // rank 7
    S(-29, -14), S(  4, -21), S(-82, -11), S(-37,  -8), S(-25,  -7), S(-42,  -9), S(  7, -17), S( -8, -24),  // rank 8
};

constexpr Score ROOK_PST[64] = {
    S(-19,  -9), S(-13,   2), S(  1,   3), S( 17,  -1), S( 16,  -5), S(  7, -13), S(-37,   4), S(-26, -20),  // rank 1
    S(-44,  -6), S(-16,  -6), S(-20,   0), S( -9,   2), S( -1,  -9), S( 11,  -9), S( -6, -11), S(-71,  -3),  // rank 2
    S(-45,  -4), S(-25,   0), S(-16,  -5), S(-17,  -1), S(  3,  -7), S(  0, -12), S( -5,  -8), S(-33, -16),  // rank 3
    S(-36,   3), S(-26,   5), S(-12,   8), S( -1,   4), S(  9,  -5), S( -7,  -6), S(  6,  -8), S(-23, -11),  // rank 4
    S(-24,   4), S(-11,   3), S(  7,  13), S( 26,   1), S( 24,   2), S( 35,   1), S( -8,  -1), S(-20,   2),  // rank 5
    S( -5,   7), S( 19,   7), S( 26,   7), S( 36,   5), S( 17,   4), S( 45,  -3), S( 61,  -5), S( 16,  -3),  // rank 6
    S( 27,  11), S( 32,  13), S( 58,  13), S( 62,  11), S( 80,  -3), S( 67,   3), S( 26,   8), S( 44,   3),  // rank 7
    S( 32,  13), S( 42,  10), S( 32,  18), S( 51,  15), S( 63,  12), S(  9,  12), S( 31,   8), S( 43,   5),  // rank 8
};

constexpr Score QUEEN_PST[64] = {
    S( -1, -33), S(-18, -28), S( -9, -22), S( 10, -43), S(-15,  -5), S(-25, -32), S(-31, -20), S(-50, -41),  // rank 1
    S(-35, -22), S( -8, -23), S( 11, -30), S(  2, -16), S(  8, -16), S( 15, -23), S( -3, -36), S(  1, -32),  // rank 2
    S(-14, -16), S(  2, -27), S(-11,  15), S( -2,   6), S( -5,   9), S(  2,  17), S( 14,  10), S(  5,   5),  // rank 3
    S( -9, -18), S(-26,  28), S( -9,  19), S(-10,  47), S( -2,  31), S( -4,  34), S(  3,  39), S( -3,  23),  // rank 4
    S(-27,   3), S(-27,  22), S(-16,  24), S(-16,  45), S( -1,  57), S( 17,  40), S( -2,  57), S(  1,  36),  // rank 5
    S(-13, -20), S(-17,   6), S(  7,   9), S(  8,  49), S( 29,  47), S( 56,  35), S( 47,  19), S( 57,   9),  // rank 6
    S(-24, -17), S(-39,  20), S( -5,  32), S(  1,  41), S(-16,  58), S( 57,  25), S( 28,  30), S( 54,   0),  // rank 7
    S(-28,  -9), S(  0,  22), S( 29,  22), S( 12,  27), S( 59,  27), S( 44,  19), S( 43,  10), S( 45,  20),  // rank 8
};

constexpr Score KING_PST[64] = {
    S(-15, -53), S( 36, -34), S( 12, -21), S(-54, -11), S(  8, -28), S(-28, -14), S( 24, -24), S( 14, -43),  // rank 1
    S(  1, -27), S(  7, -11), S( -8,   4), S(-64,  13), S(-43,  14), S(-16,   4), S(  9,  -5), S(  8, -17),  // rank 2
    S(-14, -19), S(-14,  -3), S(-22,  11), S(-46,  21), S(-44,  23), S(-30,  16), S(-15,   7), S(-27,  -9),  // rank 3
    S(-49, -18), S( -1,  -4), S(-27,  21), S(-39,  24), S(-46,  27), S(-44,  23), S(-33,   9), S(-51, -11),  // rank 4
    S(-17,  -8), S(-20,  22), S(-12,  24), S(-27,  27), S(-30,  26), S(-25,  33), S(-14,  26), S(-36,   3),  // rank 5
    S( -9,  10), S( 24,  17), S(  2,  23), S(-16,  15), S(-20,  20), S(  6,  45), S( 22,  44), S(-22,  13),  // rank 6
    S( 29, -12), S( -1,  17), S(-20,  14), S( -7,  17), S( -8,  17), S( -4,  38), S(-38,  23), S(-29,  11),  // rank 7
    S(-65, -74), S( 23, -35), S( 16, -18), S(-15, -18), S(-56, -11), S(-34,  15), S(  2,   4), S( 13, -17),  // rank 8
};
// clang-format on

constexpr const Score* PST[] = {PAWN_PST, KNIGHT_PST, BISHOP_PST, ROOK_PST, QUEEN_PST, KING_PST};

// Phase weights for non-pawn pieces (total = 24)
constexpr int PHASE_WEIGHT[] = {0, 1, 1, 2, 4, 0};
constexpr int TOTAL_PHASE = 24;

// Adjacent file masks for pawn structure evaluation
constexpr Bitboard ADJ_FILES[8] = {
    FILE_B_BB,
    FILE_A_BB | FILE_C_BB,
    FILE_B_BB | FILE_D_BB,
    FILE_C_BB | FILE_E_BB,
    FILE_D_BB | FILE_F_BB,
    FILE_E_BB | FILE_G_BB,
    FILE_F_BB | FILE_H_BB,
    FILE_G_BB,
};

// Bonus / penalty values
constexpr Score BISHOP_PAIR_BONUS = S(30, 40);
constexpr Score DOUBLED_PAWN_PENALTY = S(-10, -15);
constexpr Score ISOLATED_PAWN_PENALTY = S(-15, -20);
constexpr Score PASSED_PAWN_BONUS[8] = {S(0, 0),   S(5, 10),  S(10, 20),   S(20, 35),
                                        S(35, 55), S(60, 90), S(100, 150), S(0, 0)};
constexpr Score ROOK_OPEN_FILE_BONUS = S(15, 10);
constexpr Score ROOK_SEMI_OPEN_FILE_BONUS = S(8, 5);

// Mobility: MG/EG score per move above/below baseline
constexpr Score KNIGHT_MOB_WEIGHT = S(4, 4);
constexpr int KNIGHT_MOB_BASELINE = 4;
constexpr Score BISHOP_MOB_WEIGHT = S(3, 3);
constexpr int BISHOP_MOB_BASELINE = 7;
constexpr Score ROOK_MOB_WEIGHT = S(2, 2);
constexpr int ROOK_MOB_BASELINE = 7;
constexpr Score QUEEN_MOB_WEIGHT = S(1, 1);
constexpr int QUEEN_MOB_BASELINE = 14;

// King safety
// Pawn shelter: bonus per pawn on king's file or adjacent file within 2 ranks ahead
constexpr Score SHELTER_PAWN_BONUS = S(15, 0);
constexpr Score SHELTER_STORM_PENALTY = S(-10, 0);
// Semi-open file near king (no own pawn, enemy pawn present): shelter gap + active storm threat
constexpr Score KING_SEMI_OPEN_FILE_PENALTY = S(-20, 0);
// Open file near king (no pawns at all): shelter gap only, no storm
constexpr Score KING_OPEN_FILE_PENALTY = S(-10, 0);
// Attacker weights for the king attack zone (indexed by PieceType: Pawn=0..King=5)
// Knights are weighted highest — they leap past defenses and their checks are hardest to see.
// Queen is low because it will also be counted via safe checks when that is added.
constexpr int KING_ATTACKER_WEIGHT[] = {0, 7, 5, 4, 4, 0};
constexpr int KING_DANGER_DIVIDER = 8;  // penalty = danger² / KING_DANGER_DIVIDER (mg only)

Score materialAndPST(const Position& pos) {
    Score score;
    for (int pt = 0; pt < 6; ++pt) {
        PieceType pieceType = static_cast<PieceType>(pt);

        Bitboard white = pos.pieces(pieceType, Color::White);
        while (white) {
            Square sq = popLsb(white);
            score += MATERIAL_VALUE[pt] + PST[pt][sq];
        }

        Bitboard black = pos.pieces(pieceType, Color::Black);
        while (black) {
            Square sq = popLsb(black);
            score -= MATERIAL_VALUE[pt] + PST[pt][sq ^ 56];
        }
    }
    return score;
}

int gamePhase(const Position& pos) {
    int phase = 0;
    for (int pt = 1; pt < 5; ++pt) {
        PieceType pieceType = static_cast<PieceType>(pt);
        phase += PHASE_WEIGHT[pt] * popCount(pos.pieces(pieceType, Color::White));
        phase += PHASE_WEIGHT[pt] * popCount(pos.pieces(pieceType, Color::Black));
    }
    return std::min(phase, TOTAL_PHASE);
}

Score bishopPair(const Position& pos) {
    Score score;
    if (popCount(pos.pieces(PieceType::Bishop, Color::White)) >= 2)
        score += BISHOP_PAIR_BONUS;
    if (popCount(pos.pieces(PieceType::Bishop, Color::Black)) >= 2)
        score -= BISHOP_PAIR_BONUS;
    return score;
}

Score pawnStructure(Bitboard wp, Bitboard bp) {
    Score score;
    for (int f = 0; f < 8; ++f) {
        Bitboard fileMask = FILE_BB[f];
        int wCount = popCount(wp & fileMask);
        int bCount = popCount(bp & fileMask);

        if (wCount > 1)
            score += (wCount - 1) * DOUBLED_PAWN_PENALTY;
        if (bCount > 1)
            score -= (bCount - 1) * DOUBLED_PAWN_PENALTY;

        if (wCount && !(wp & ADJ_FILES[f]))
            score += wCount * ISOLATED_PAWN_PENALTY;
        if (bCount && !(bp & ADJ_FILES[f]))
            score -= bCount * ISOLATED_PAWN_PENALTY;
    }
    return score;
}

Score passedPawns(Bitboard wp, Bitboard bp) {
    Score score;

    Bitboard wPawns = wp;
    while (wPawns) {
        Square sq = popLsb(wPawns);
        File f = getFile(sq);
        Rank r = getRank(sq);
        Bitboard mask = FILE_BB[f] | ADJ_FILES[f];
        for (int ri = 0; ri <= r; ++ri)
            mask &= ~RANK_BB[ri];
        if (!(bp & mask))
            score += PASSED_PAWN_BONUS[r];
    }

    Bitboard bPawns = bp;
    while (bPawns) {
        Square sq = popLsb(bPawns);
        File f = getFile(sq);
        Rank r = getRank(sq);
        Bitboard mask = FILE_BB[f] | ADJ_FILES[f];
        for (int ri = r; ri < 8; ++ri)
            mask &= ~RANK_BB[ri];
        if (!(wp & mask))
            score -= PASSED_PAWN_BONUS[7 - r];
    }

    return score;
}

Score rookOpenFiles(const Position& pos, Bitboard wp, Bitboard bp) {
    Score score;

    Bitboard wRooks = pos.pieces(PieceType::Rook, Color::White);
    while (wRooks) {
        Bitboard fileMask = FILE_BB[getFile(popLsb(wRooks))];
        if (!(wp & fileMask))
            score += (bp & fileMask) ? ROOK_SEMI_OPEN_FILE_BONUS : ROOK_OPEN_FILE_BONUS;
    }

    Bitboard bRooks = pos.pieces(PieceType::Rook, Color::Black);
    while (bRooks) {
        Bitboard fileMask = FILE_BB[getFile(popLsb(bRooks))];
        if (!(bp & fileMask))
            score -= (wp & fileMask) ? ROOK_SEMI_OPEN_FILE_BONUS : ROOK_OPEN_FILE_BONUS;
    }

    return score;
}

Score pieceEval(const Position& pos, Bitboard wp, Bitboard bp, EvalState& state) {
    Score score;
    Bitboard occupied = pos.occupied();

    // ---- Pawn attacks ----
    state.pawnAtk[static_cast<int>(Color::White)] = shiftNorthEast(wp) | shiftNorthWest(wp);
    state.pawnAtk[static_cast<int>(Color::Black)] = shiftSouthEast(bp) | shiftSouthWest(bp);

    // Seed king and pawn attacks into the map.
    for (int ci = 0; ci < 2; ++ci) {
        Color c = static_cast<Color>(ci);
        Square kSq = pos.kingSquare(c);
        Bitboard kAtk = KING_ATTACKS[kSq];
        state.attackedBy[ci][static_cast<int>(PieceType::King)] = kAtk;
        state.attacked[ci] |= kAtk;
        state.attacked[ci] |= state.pawnAtk[ci];
        state.attackedBy[ci][static_cast<int>(PieceType::Pawn)] = state.pawnAtk[ci];
    }

    // Mobility area: exclude own pieces and squares controlled by enemy pawns
    const Bitboard wArea =
        ~(pos.pieces(Color::White) | state.pawnAtk[static_cast<int>(Color::Black)]);
    const Bitboard bArea =
        ~(pos.pieces(Color::Black) | state.pawnAtk[static_cast<int>(Color::White)]);
    const Bitboard mobArea[2] = {wArea, bArea};

    // ---- Knights ----
    for (int ci = 0; ci < 2; ++ci) {
        Color c = static_cast<Color>(ci);
        int sign = (ci == 0) ? 1 : -1;
        Bitboard knights = pos.pieces(PieceType::Knight, c);
        while (knights) {
            Square sq = popLsb(knights);
            Bitboard atk = KNIGHT_ATTACKS[sq];
            state.attackedBy[ci][static_cast<int>(PieceType::Knight)] |= atk;
            state.attacked[ci] |= atk;
            int mob = popCount(atk & mobArea[ci]);
            score += sign * ((mob - KNIGHT_MOB_BASELINE) * KNIGHT_MOB_WEIGHT);
        }
    }

    // ---- Bishops ----
    for (int ci = 0; ci < 2; ++ci) {
        Color c = static_cast<Color>(ci);
        int sign = (ci == 0) ? 1 : -1;
        Bitboard bishops = pos.pieces(PieceType::Bishop, c);
        while (bishops) {
            Square sq = popLsb(bishops);
            Bitboard atk = bishopAttacks(sq, occupied);  // 1 magic lookup
            state.attackedBy[ci][static_cast<int>(PieceType::Bishop)] |= atk;
            state.attacked[ci] |= atk;
            int mob = popCount(atk & mobArea[ci]);
            score += sign * ((mob - BISHOP_MOB_BASELINE) * BISHOP_MOB_WEIGHT);
        }
    }

    // ---- Rooks ----
    for (int ci = 0; ci < 2; ++ci) {
        Color c = static_cast<Color>(ci);
        int sign = (ci == 0) ? 1 : -1;
        Bitboard rooks = pos.pieces(PieceType::Rook, c);
        while (rooks) {
            Square sq = popLsb(rooks);
            Bitboard atk = rookAttacks(sq, occupied);  // 1 magic lookup
            state.attackedBy[ci][static_cast<int>(PieceType::Rook)] |= atk;
            state.attacked[ci] |= atk;
            int mob = popCount(atk & mobArea[ci]);
            score += sign * ((mob - ROOK_MOB_BASELINE) * ROOK_MOB_WEIGHT);
        }
    }

    // ---- Queens ----
    for (int ci = 0; ci < 2; ++ci) {
        Color c = static_cast<Color>(ci);
        int sign = (ci == 0) ? 1 : -1;
        Bitboard queens = pos.pieces(PieceType::Queen, c);
        while (queens) {
            Square sq = popLsb(queens);
            Bitboard atk = rookAttacks(sq, occupied) | bishopAttacks(sq, occupied);  // 2 lookups
            state.attackedBy[ci][static_cast<int>(PieceType::Queen)] |= atk;
            state.attacked[ci] |= atk;
            int mob = popCount(atk & mobArea[ci]);
            score += sign * ((mob - QUEEN_MOB_BASELINE) * QUEEN_MOB_WEIGHT);
        }
    }

    return score;
}

Score mobility(const Position& pos) {
    Bitboard wp = pos.pieces(PieceType::Pawn, Color::White);
    Bitboard bp = pos.pieces(PieceType::Pawn, Color::Black);
    EvalState state;
    return pieceEval(pos, wp, bp, state);
}

// Returns a bitboard of the 3x3 king zone (king square + all adjacent squares).
static inline Bitboard kingZone(Square kingSq) {
    return KING_ATTACKS[kingSq] | squareBB(kingSq);
}

Score kingSafety(const Position& pos, Bitboard wp, Bitboard bp, const EvalState& state) {
    Score score;

    for (int ci = 0; ci < 2; ++ci) {
        Color us = static_cast<Color>(ci);
        int them = ci ^ 1;

        Square kSq = pos.kingSquare(us);
        File kFile = getFile(kSq);
        Rank kRank = getRank(kSq);
        Bitboard zone = kingZone(kSq);

        // ---- 1 & 2. Pawn shelter, storm, and open files ----
        Bitboard ownPawns = (ci == 0) ? wp : bp;
        Bitboard enemyPawns = (ci == 0) ? bp : wp;

        // Build the "ahead" rank mask once — two ranks in front of the king.
        Bitboard aheadRanks = BB_EMPTY;
        if (ci == 0) {
            for (int r = kRank + 1; r <= std::min(7, kRank + 2); ++r)
                aheadRanks |= RANK_BB[r];
        } else {
            for (int r = std::max(0, kRank - 2); r < kRank; ++r)
                aheadRanks |= RANK_BB[r];
        }

        int fileStart = std::max(0, static_cast<int>(kFile) - 1);
        int fileEnd = std::min(7, static_cast<int>(kFile) + 1);

        int shelterBonus = 0;
        int stormPenalty = 0;
        Score termFiles;

        for (int f = fileStart; f <= fileEnd; ++f) {
            Bitboard fileMask = FILE_BB[f];

            if (ownPawns & fileMask & aheadRanks)
                shelterBonus += 1;
            if (enemyPawns & fileMask & aheadRanks)
                stormPenalty += 1;

            if (!(ownPawns & fileMask)) {
                termFiles +=
                    (enemyPawns & fileMask) ? KING_SEMI_OPEN_FILE_PENALTY : KING_OPEN_FILE_PENALTY;
            }
        }

        Score termShelter =
            shelterBonus * SHELTER_PAWN_BONUS + stormPenalty * SHELTER_STORM_PENALTY;

        // ---- 3. Attacker danger in king zone ----
        int danger = 0;
        danger += KING_ATTACKER_WEIGHT[static_cast<int>(PieceType::Knight)] *
                  popCount(state.attackedBy[them][static_cast<int>(PieceType::Knight)] & zone);
        danger += KING_ATTACKER_WEIGHT[static_cast<int>(PieceType::Bishop)] *
                  popCount(state.attackedBy[them][static_cast<int>(PieceType::Bishop)] & zone);
        danger += KING_ATTACKER_WEIGHT[static_cast<int>(PieceType::Rook)] *
                  popCount(state.attackedBy[them][static_cast<int>(PieceType::Rook)] & zone);
        danger += KING_ATTACKER_WEIGHT[static_cast<int>(PieceType::Queen)] *
                  popCount(state.attackedBy[them][static_cast<int>(PieceType::Queen)] & zone);

        // Quadratic penalty, MG only.
        int dangerPenalty = (danger * danger) / KING_DANGER_DIVIDER;
        Score termDanger = S(-dangerPenalty, 0);

        Score total = termShelter + termFiles + termDanger;
        if (ci == 0)
            score += total;
        else
            score -= total;
    }

    return score;
}

int evaluate(const Position& pos) {
    Bitboard wp = pos.pieces(PieceType::Pawn, Color::White);
    Bitboard bp = pos.pieces(PieceType::Pawn, Color::Black);

    EvalState state;
    Score score = materialAndPST(pos) + bishopPair(pos) + pawnStructure(wp, bp) +
                  passedPawns(wp, bp) + rookOpenFiles(pos, wp, bp) + pieceEval(pos, wp, bp, state) +
                  kingSafety(pos, wp, bp, state);

    // Taper: interpolate between MG and EG based on phase
    int phase = gamePhase(pos);
    int tapered = (score.mg * phase + score.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return (pos.sideToMove() == Color::White) ? tapered : -tapered;
}

}  // namespace eval
}  // namespace cchess
