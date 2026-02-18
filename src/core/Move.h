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
    Move()
        : from_(SQUARE_NONE),
          to_(SQUARE_NONE),
          type_(MoveType::Normal),
          promotion_(PieceType::None) {}

    // Standard move constructor
    Move(Square from, Square to, MoveType type = MoveType::Normal)
        : from_(from), to_(to), type_(type), promotion_(PieceType::None) {}

    // Promotion move constructor
    Move(Square from, Square to, MoveType type, PieceType promotion)
        : from_(from), to_(to), type_(type), promotion_(promotion) {}

    // Factory methods for special moves
    static Move makePromotion(Square from, Square to, PieceType promotion) {
        return Move(from, to, MoveType::Promotion, promotion);
    }
    static Move makePromotionCapture(Square from, Square to, PieceType promotion) {
        return Move(from, to, MoveType::PromotionCapture, promotion);
    }
    static Move makeCastling(Square from, Square to) { return Move(from, to, MoveType::Castling); }
    static Move makeEnPassant(Square from, Square to) {
        return Move(from, to, MoveType::EnPassant);
    }

    // Accessors
    Square from() const { return from_; }
    Square to() const { return to_; }
    MoveType type() const { return type_; }
    PieceType promotion() const { return promotion_; }

    // Move classification helpers
    bool isCapture() const {
        return type_ == MoveType::Capture || type_ == MoveType::PromotionCapture ||
               type_ == MoveType::EnPassant;
    }
    bool isPromotion() const {
        return type_ == MoveType::Promotion || type_ == MoveType::PromotionCapture;
    }
    bool isCastling() const { return type_ == MoveType::Castling; }
    bool isEnPassant() const { return type_ == MoveType::EnPassant; }
    bool isNull() const { return from_ == SQUARE_NONE || to_ == SQUARE_NONE; }

    // Comparison operators
    bool operator==(const Move& other) const {
        return from_ == other.from_ && to_ == other.to_ && type_ == other.type_ &&
               promotion_ == other.promotion_;
    }
    bool operator!=(const Move& other) const { return !(*this == other); }

    // String conversions
    std::string toAlgebraic() const;
    static std::optional<Move> fromAlgebraic(const std::string& str);

private:
    Square from_;
    Square to_;
    MoveType type_;
    PieceType promotion_;  // Only used for promotion moves
};

}  // namespace cchess

#endif  // CCHESS_MOVE_H
