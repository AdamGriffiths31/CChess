#include "ai/Eval.h"

#include "core/Bitboard.h"
#include "core/movegen/AttackTables.h"

#include <algorithm>

namespace cchess {
namespace eval {

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
    Score score = pos.psqt() + bishopPair(pos) + pawnStructure(wp, bp) +
                  passedPawns(wp, bp) + rookOpenFiles(pos, wp, bp) + pieceEval(pos, wp, bp, state) +
                  kingSafety(pos, wp, bp, state);

    // Taper: interpolate between MG and EG based on phase
    int phase = gamePhase(pos);
    int tapered = (score.mg * phase + score.eg * (TOTAL_PHASE - phase)) / TOTAL_PHASE;

    return (pos.sideToMove() == Color::White) ? tapered : -tapered;
}

}  // namespace eval
}  // namespace cchess
