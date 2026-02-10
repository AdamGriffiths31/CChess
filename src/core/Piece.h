#ifndef CCHESS_PIECE_H
#define CCHESS_PIECE_H

#include "Types.h"

#include <string>

namespace cchess {

class Piece {
public:
    // Constructors
    Piece() : type_(PieceType::None), color_(Color::None) {}
    Piece(PieceType type, Color color) : type_(type), color_(color) {}

    // Getters
    PieceType type() const { return type_; }
    Color color() const { return color_; }
    bool isEmpty() const { return type_ == PieceType::None; }
    bool isValid() const {
        // Either empty or both type and color must be valid
        return (type_ == PieceType::None && color_ == Color::None) ||
               (pieceTypeIsValid(type_) && colorIsValid(color_));
    }

    // FEN conversion
    char toFenChar() const;
    static Piece fromFenChar(char c);

    // Unicode representation
    std::string toUnicode() const;

    // ASCII representation
    char toAscii() const;

    // Equality operators
    bool operator==(const Piece& other) const {
        return type_ == other.type_ && color_ == other.color_;
    }

    bool operator!=(const Piece& other) const { return !(*this == other); }

private:
    PieceType type_;
    Color color_;
};

}  // namespace cchess

#endif  // CCHESS_PIECE_H
