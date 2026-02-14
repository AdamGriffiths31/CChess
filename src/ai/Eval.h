#ifndef CCHESS_EVAL_H
#define CCHESS_EVAL_H

#include "core/Bitboard.h"
#include "core/Position.h"

namespace cchess {
namespace eval {

constexpr int SCORE_MATE = 100000;
constexpr int SCORE_INFINITY = 200000;
constexpr int SCORE_DRAW = 0;

// Returns score relative to side to move (positive = good for side to move)
int evaluate(const Position& pos);

// Individual eval terms (white-relative, not side-to-move-relative)
int materialAndPST(const Position& pos);
int bishopPair(const Position& pos);
int pawnStructure(Bitboard wp, Bitboard bp);
int passedPawns(Bitboard wp, Bitboard bp);
int rookOpenFiles(const Position& pos, Bitboard wp, Bitboard bp);
int mobility(const Position& pos);

}  // namespace eval
}  // namespace cchess

#endif  // CCHESS_EVAL_H
