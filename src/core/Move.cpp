#include "Move.h"

#include "Square.h"

#include <cctype>

namespace cchess {

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

}  // namespace cchess
