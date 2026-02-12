#ifndef CCHESS_EVAL_H
#define CCHESS_EVAL_H

#include "core/Position.h"

namespace cchess {
namespace eval {

constexpr int SCORE_MATE = 100000;
constexpr int SCORE_INFINITY = 200000;
constexpr int SCORE_DRAW = 0;

// Returns score relative to side to move (positive = good for side to move)
int evaluate(const Position& pos);

}  // namespace eval
}  // namespace cchess

#endif  // CCHESS_EVAL_H
