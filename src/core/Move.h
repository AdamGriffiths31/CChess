#ifndef CCHESS_MOVE_H
#define CCHESS_MOVE_H

#include "Types.h"

#include <optional>
#include <string>

namespace cchess {

// Move type enumeration
enum class MoveType : uint8_t {
    Normal = 0,           // Regular move (non-capture)
    Capture = 1,          // Capture move
    EnPassant = 2,        // En passant capture
    Castling = 3,         // Castling (kingside or queenside)
    Promotion = 4,        // Pawn promotion (non-capture)
    PromotionCapture = 5  // Pawn promotion with capture
};

class Move {
public:
    // Default constructor (creates a null move)
    Move();

    // Standard move constructor
    Move(Square from, Square to, MoveType type = MoveType::Normal);

    // Promotion move constructor
    Move(Square from, Square to, MoveType type, PieceType promotion);

    // Factory methods for special moves
    static Move makePromotion(Square from, Square to, PieceType promotion);
    static Move makePromotionCapture(Square from, Square to, PieceType promotion);
    static Move makeCastling(Square from, Square to);
    static Move makeEnPassant(Square from, Square to);

    // Accessors
    Square from() const { return from_; }
    Square to() const { return to_; }
    MoveType type() const { return type_; }
    PieceType promotion() const { return promotion_; }

    // Move classification helpers
    bool isCapture() const;
    bool isPromotion() const;
    bool isCastling() const;
    bool isEnPassant() const;
    bool isNull() const;

    // String conversions
    std::string toAlgebraic() const;
    static std::optional<Move> fromAlgebraic(const std::string& str);

    // Comparison operators
    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const;

private:
    Square from_;
    Square to_;
    MoveType type_;
    PieceType promotion_;  // Only used for promotion moves
};

}  // namespace cchess

#endif  // CCHESS_MOVE_H
