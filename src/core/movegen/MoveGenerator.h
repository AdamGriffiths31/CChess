#ifndef CCHESS_MOVE_GENERATOR_H
#define CCHESS_MOVE_GENERATOR_H

#include "../Move.h"
#include "../MoveList.h"
#include "../Position.h"

namespace cchess {

// Forward declarations
class Board;

class MoveGenerator {
public:
    // Main move generation functions
    static MoveList generateLegalMoves(const Position& pos);
    static MoveList generatePseudoLegalMoves(const Position& pos);

    // Move validation
    static bool isLegal(const Position& pos, const Move& move);

    // Check detection
    static bool isSquareAttacked(const Position& pos, Square sq, Color byColor);
    static bool isInCheck(const Position& pos, Color side);
    static bool moveLeavesKingInCheck(Position& pos, const Move& move);

    // Game state queries
    static bool isCheckmate(const Position& pos);
    static bool isStalemate(const Position& pos);
    static bool isDraw(const Position& pos);  // 50-move rule

private:
    // Per-piece move generators (pseudo-legal)
    static void generatePawnMoves(const Position& pos, Square from, MoveList& moves);
    static void generateKnightMoves(const Position& pos, Square from, MoveList& moves);
    static void generateBishopMoves(const Position& pos, Square from, MoveList& moves);
    static void generateRookMoves(const Position& pos, Square from, MoveList& moves);
    static void generateQueenMoves(const Position& pos, Square from, MoveList& moves);
    static void generateKingMoves(const Position& pos, Square from, MoveList& moves);
    static void generateCastlingMoves(const Position& pos, MoveList& moves);

    // Helper to find king square
    static Square findKingSquare(const Position& pos, Color color);
};

}  // namespace cchess

#endif  // CCHESS_MOVE_GENERATOR_H
