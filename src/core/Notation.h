#ifndef CCHESS_NOTATION_H
#define CCHESS_NOTATION_H

#include "Move.h"

#include <string>

namespace cchess {

class Board;

// Convert a move to Standard Algebraic Notation (e.g., Nf3, exd5, O-O, e8=Q+)
// The board must reflect the position BEFORE the move is made.
std::string moveToSan(const Board& board, const Move& move);

}  // namespace cchess

#endif  // CCHESS_NOTATION_H
