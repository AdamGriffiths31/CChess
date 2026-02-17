#include "ai/Eval.h"

#include "core/Bitboard.h"
#include "core/movegen/AttackTables.h"

namespace cchess {
namespace eval {

// Material values
constexpr int MATERIAL_VALUE[] = {
    100,  // Pawn
    320,  // Knight
    330,  // Bishop
    500,  // Rook
    900,  // Queen
    0     // King (not counted)
};

// Piece-square tables from White's perspective (a1 = index 0).
// For Black, mirror via sq ^ 56.

// clang-format off
constexpr int PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,   // rank 1
     5, 10, 10,-20,-20, 10, 10,  5,   // rank 2
     5, -5,-10,  0,  0,-10, -5,  5,   // rank 3
     0,  0,  0, 20, 20,  0,  0,  0,   // rank 4
     5,  5, 10, 25, 25, 10,  5,  5,   // rank 5
    10, 10, 20, 30, 30, 20, 10, 10,   // rank 6
    50, 50, 50, 50, 50, 50, 50, 50,   // rank 7
     0,  0,  0,  0,  0,  0,  0,  0    // rank 8
};

constexpr int KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,   // rank 1
    -40,-20,  0,  5,  5,  0,-20,-40,   // rank 2
    -30,  5, 10, 15, 15, 10,  5,-30,   // rank 3
    -30,  0, 15, 20, 20, 15,  0,-30,   // rank 4
    -30,  5, 15, 20, 20, 15,  5,-30,   // rank 5
    -30,  0, 10, 15, 15, 10,  0,-30,   // rank 6
    -40,-20,  0,  0,  0,  0,-20,-40,   // rank 7
    -50,-40,-30,-30,-30,-30,-40,-50    // rank 8
};

constexpr int BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,   // rank 1
    -10,  5,  0,  0,  0,  0,  5,-10,   // rank 2
    -10, 10, 10, 10, 10, 10, 10,-10,   // rank 3
    -10,  0, 10, 10, 10, 10,  0,-10,   // rank 4
    -10,  5,  5, 10, 10,  5,  5,-10,   // rank 5
    -10,  0,  5, 10, 10,  5,  0,-10,   // rank 6
    -10,  0,  0,  0,  0,  0,  0,-10,   // rank 7
    -20,-10,-10,-10,-10,-10,-10,-20    // rank 8
};

constexpr int ROOK_PST[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,   // rank 1
    -5,  0,  0,  0,  0,  0,  0, -5,   // rank 2
    -5,  0,  0,  0,  0,  0,  0, -5,   // rank 3
    -5,  0,  0,  0,  0,  0,  0, -5,   // rank 4
    -5,  0,  0,  0,  0,  0,  0, -5,   // rank 5
    -5,  0,  0,  0,  0,  0,  0, -5,   // rank 6
     5, 10, 10, 10, 10, 10, 10,  5,   // rank 7
     0,  0,  0,  0,  0,  0,  0,  0    // rank 8
};

constexpr int QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,   // rank 1
    -10,  0,  5,  0,  0,  0,  0,-10,   // rank 2
    -10,  5,  5,  5,  5,  5,  0,-10,   // rank 3
     -5,  0,  5,  5,  5,  5,  0, -5,   // rank 4
     -5,  0,  5,  5,  5,  5,  0, -5,   // rank 5
    -10,  0,  5,  5,  5,  5,  0,-10,   // rank 6
    -10,  0,  0,  0,  0,  0,  0,-10,   // rank 7
    -20,-10,-10, -5, -5,-10,-10,-20    // rank 8
};

constexpr int KING_PST[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,   // rank 1
     20, 20,  0,  0,  0,  0, 20, 20,   // rank 2
    -10,-20,-20,-20,-20,-20,-20,-10,   // rank 3
    -20,-30,-30,-40,-40,-30,-30,-20,   // rank 4
    -30,-40,-40,-50,-50,-40,-40,-30,   // rank 5
    -30,-40,-40,-50,-50,-40,-40,-30,   // rank 6
    -30,-40,-40,-50,-50,-40,-40,-30,   // rank 7
    -30,-40,-40,-50,-50,-40,-40,-30    // rank 8
};
// clang-format on

constexpr const int* PST[] = {PAWN_PST, KNIGHT_PST, BISHOP_PST, ROOK_PST, QUEEN_PST, KING_PST};

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
constexpr int BISHOP_PAIR_BONUS = 30;
constexpr int DOUBLED_PAWN_PENALTY = -10;
constexpr int ISOLATED_PAWN_PENALTY = -15;
constexpr int PASSED_PAWN_BONUS[8] = {0, 5, 10, 20, 35, 60, 100, 0};
constexpr int ROOK_OPEN_FILE_BONUS = 15;
constexpr int ROOK_SEMI_OPEN_FILE_BONUS = 8;

// Mobility: centipawns per move above/below baseline (CPW-style linear)
constexpr int KNIGHT_MOB_WEIGHT = 4;
constexpr int KNIGHT_MOB_BASELINE = 4;
constexpr int BISHOP_MOB_WEIGHT = 3;
constexpr int BISHOP_MOB_BASELINE = 7;
constexpr int ROOK_MOB_WEIGHT = 2;
constexpr int ROOK_MOB_BASELINE = 7;
constexpr int QUEEN_MOB_WEIGHT = 1;
constexpr int QUEEN_MOB_BASELINE = 14;

int materialAndPST(const Position& pos) {
    int score = 0;
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

int bishopPair(const Position& pos) {
    int score = 0;
    score += BISHOP_PAIR_BONUS * (popCount(pos.pieces(PieceType::Bishop, Color::White)) >= 2);
    score -= BISHOP_PAIR_BONUS * (popCount(pos.pieces(PieceType::Bishop, Color::Black)) >= 2);
    return score;
}

int pawnStructure(Bitboard wp, Bitboard bp) {
    int score = 0;
    for (int f = 0; f < 8; ++f) {
        Bitboard fileMask = FILE_BB[f];
        int wCount = popCount(wp & fileMask);
        int bCount = popCount(bp & fileMask);

        if (wCount > 1)
            score += DOUBLED_PAWN_PENALTY * (wCount - 1);
        if (bCount > 1)
            score -= DOUBLED_PAWN_PENALTY * (bCount - 1);

        if (wCount && !(wp & ADJ_FILES[f]))
            score += ISOLATED_PAWN_PENALTY * wCount;
        if (bCount && !(bp & ADJ_FILES[f]))
            score -= ISOLATED_PAWN_PENALTY * bCount;
    }
    return score;
}

int passedPawns(Bitboard wp, Bitboard bp) {
    int score = 0;

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

int rookOpenFiles(const Position& pos, Bitboard wp, Bitboard bp) {
    int score = 0;

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

int mobility(const Position& pos) {
    int score = 0;
    Bitboard occupied = pos.occupied();

    // Mobility area: exclude friendly pieces and squares attacked by enemy pawns
    Bitboard wPawnAtk = shiftNorthEast(pos.pieces(PieceType::Pawn, Color::White)) |
                        shiftNorthWest(pos.pieces(PieceType::Pawn, Color::White));
    Bitboard bPawnAtk = shiftSouthEast(pos.pieces(PieceType::Pawn, Color::Black)) |
                        shiftSouthWest(pos.pieces(PieceType::Pawn, Color::Black));

    Bitboard wArea = ~(pos.pieces(Color::White) | bPawnAtk);
    Bitboard bArea = ~(pos.pieces(Color::Black) | wPawnAtk);

    // Knights
    Bitboard wKnights = pos.pieces(PieceType::Knight, Color::White);
    while (wKnights) {
        int mob = popCount(KNIGHT_ATTACKS[popLsb(wKnights)] & wArea);
        score += KNIGHT_MOB_WEIGHT * (mob - KNIGHT_MOB_BASELINE);
    }
    Bitboard bKnights = pos.pieces(PieceType::Knight, Color::Black);
    while (bKnights) {
        int mob = popCount(KNIGHT_ATTACKS[popLsb(bKnights)] & bArea);
        score -= KNIGHT_MOB_WEIGHT * (mob - KNIGHT_MOB_BASELINE);
    }

    // Bishops
    Bitboard wBishops = pos.pieces(PieceType::Bishop, Color::White);
    while (wBishops) {
        int mob = popCount(bishopAttacks(popLsb(wBishops), occupied) & wArea);
        score += BISHOP_MOB_WEIGHT * (mob - BISHOP_MOB_BASELINE);
    }
    Bitboard bBishops = pos.pieces(PieceType::Bishop, Color::Black);
    while (bBishops) {
        int mob = popCount(bishopAttacks(popLsb(bBishops), occupied) & bArea);
        score -= BISHOP_MOB_WEIGHT * (mob - BISHOP_MOB_BASELINE);
    }

    // Rooks
    Bitboard wRooks = pos.pieces(PieceType::Rook, Color::White);
    while (wRooks) {
        int mob = popCount(rookAttacks(popLsb(wRooks), occupied) & wArea);
        score += ROOK_MOB_WEIGHT * (mob - ROOK_MOB_BASELINE);
    }
    Bitboard bRooks = pos.pieces(PieceType::Rook, Color::Black);
    while (bRooks) {
        int mob = popCount(rookAttacks(popLsb(bRooks), occupied) & bArea);
        score -= ROOK_MOB_WEIGHT * (mob - ROOK_MOB_BASELINE);
    }

    // Queens
    Bitboard wQueens = pos.pieces(PieceType::Queen, Color::White);
    while (wQueens) {
        Square sq = popLsb(wQueens);
        int mob = popCount((rookAttacks(sq, occupied) | bishopAttacks(sq, occupied)) & wArea);
        score += QUEEN_MOB_WEIGHT * (mob - QUEEN_MOB_BASELINE);
    }
    Bitboard bQueens = pos.pieces(PieceType::Queen, Color::Black);
    while (bQueens) {
        Square sq = popLsb(bQueens);
        int mob = popCount((rookAttacks(sq, occupied) | bishopAttacks(sq, occupied)) & bArea);
        score -= QUEEN_MOB_WEIGHT * (mob - QUEEN_MOB_BASELINE);
    }

    return score;
}

int evaluate(const Position& pos) {
    Bitboard wp = pos.pieces(PieceType::Pawn, Color::White);
    Bitboard bp = pos.pieces(PieceType::Pawn, Color::Black);

    int score = materialAndPST(pos) + bishopPair(pos) + pawnStructure(wp, bp) +
                passedPawns(wp, bp) + rookOpenFiles(pos, wp, bp) + mobility(pos);

    return (pos.sideToMove() == Color::White) ? score : -score;
}

}  // namespace eval
}  // namespace cchess
