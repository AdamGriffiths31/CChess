#include "Move.h"

#include "Square.h"

#include <cctype>

namespace cchess {

Move::Move()
    : from_(SQUARE_NONE), to_(SQUARE_NONE), type_(MoveType::Normal), promotion_(PieceType::None) {}

Move::Move(Square from, Square to, MoveType type)
    : from_(from), to_(to), type_(type), promotion_(PieceType::None) {}

Move::Move(Square from, Square to, MoveType type, PieceType promotion)
    : from_(from), to_(to), type_(type), promotion_(promotion) {}

Move Move::makePromotion(Square from, Square to, PieceType promotion) {
    return Move(from, to, MoveType::Promotion, promotion);
}

Move Move::makePromotionCapture(Square from, Square to, PieceType promotion) {
    return Move(from, to, MoveType::PromotionCapture, promotion);
}

Move Move::makeCastling(Square from, Square to) {
    return Move(from, to, MoveType::Castling);
}

Move Move::makeEnPassant(Square from, Square to) {
    return Move(from, to, MoveType::EnPassant);
}

bool Move::isCapture() const {
    return type_ == MoveType::Capture || type_ == MoveType::PromotionCapture ||
           type_ == MoveType::EnPassant;
}

bool Move::isPromotion() const {
    return type_ == MoveType::Promotion || type_ == MoveType::PromotionCapture;
}

bool Move::isCastling() const {
    return type_ == MoveType::Castling;
}

bool Move::isEnPassant() const {
    return type_ == MoveType::EnPassant;
}

bool Move::isNull() const {
    return from_ == SQUARE_NONE || to_ == SQUARE_NONE;
}

std::string Move::toAlgebraic() const {
    if (isNull()) {
        return "0000";
    }

    std::string result = squareToString(from_) + squareToString(to_);

    // Add promotion piece if applicable
    if (isPromotion()) {
        char promotionChar;
        switch (promotion_) {
            case PieceType::Queen:
                promotionChar = 'q';
                break;
            case PieceType::Rook:
                promotionChar = 'r';
                break;
            case PieceType::Bishop:
                promotionChar = 'b';
                break;
            case PieceType::Knight:
                promotionChar = 'n';
                break;
            default:
                promotionChar = 'q';
                break;  // Default to queen
        }
        result += promotionChar;
    }

    return result;
}

std::optional<Move> Move::fromAlgebraic(const std::string& str) {
    // Valid formats: "e2e4" (normal), "e7e8q" (promotion)
    if (str.length() < 4 || str.length() > 5) {
        return std::nullopt;
    }

    // Parse from and to squares
    auto fromSq = stringToSquare(str.substr(0, 2));
    auto toSq = stringToSquare(str.substr(2, 2));

    if (!fromSq || !toSq) {
        return std::nullopt;
    }

    // Check for promotion
    if (str.length() == 5) {
        char promotionChar = static_cast<char>(std::tolower(static_cast<unsigned char>(str[4])));
        PieceType promotionType;

        switch (promotionChar) {
            case 'q':
                promotionType = PieceType::Queen;
                break;
            case 'r':
                promotionType = PieceType::Rook;
                break;
            case 'b':
                promotionType = PieceType::Bishop;
                break;
            case 'n':
                promotionType = PieceType::Knight;
                break;
            default:
                return std::nullopt;  // Invalid promotion character
        }

        // Return promotion move (type will be determined later based on capture)
        return Move(*fromSq, *toSq, MoveType::Promotion, promotionType);
    }

    // Regular move (type will be determined later)
    return Move(*fromSq, *toSq, MoveType::Normal);
}

bool Move::operator==(const Move& other) const {
    return from_ == other.from_ && to_ == other.to_ && type_ == other.type_ &&
           promotion_ == other.promotion_;
}

bool Move::operator!=(const Move& other) const {
    return !(*this == other);
}

}  // namespace cchess
