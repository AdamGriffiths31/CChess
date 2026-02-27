#ifndef CCHESS_EVAL_H
#define CCHESS_EVAL_H

#include "PST.h"
#include "core/Bitboard.h"
#include "core/Position.h"

namespace cchess {
namespace eval {

constexpr int SCORE_MATE = 100000;
constexpr int SCORE_INFINITY = 200000;
constexpr int SCORE_DRAW = 0;

// Attack map built once per evaluate() call and shared across eval terms.
struct EvalState {
    Bitboard attackedBy[2][6] = {};
    Bitboard attacked[2] = {};
    Bitboard pawnAtk[2] = {};
};

// Returns score relative to side to move (positive = good for side to move)
int evaluate(const Position& pos);

// Individual eval terms (white-relative, return MG/EG score pair)
Score materialAndPST(const Position& pos);
Score bishopPair(const Position& pos);
Score pawnStructure(Bitboard wp, Bitboard bp);
Score passedPawns(Bitboard wp, Bitboard bp);
Score rookOpenFiles(const Position& pos, Bitboard wp, Bitboard bp);
Score pieceEval(const Position& pos, Bitboard wp, Bitboard bp, EvalState& state);
Score mobility(const Position& pos);
Score kingSafety(const Position& pos, Bitboard wp, Bitboard bp, const EvalState& state);

}  // namespace eval
}  // namespace cchess

#endif  // CCHESS_EVAL_H
