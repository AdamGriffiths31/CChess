#include "Piece.h"

namespace cchess {

char Piece::toFenChar() const {
    if (isEmpty())
        return ' ';

    char c;
    switch (type_) {
        case PieceType::Pawn:
            c = 'P';
            break;
        case PieceType::Knight:
            c = 'N';
            break;
        case PieceType::Bishop:
            c = 'B';
            break;
        case PieceType::Rook:
            c = 'R';
            break;
        case PieceType::Queen:
            c = 'Q';
            break;
        case PieceType::King:
            c = 'K';
            break;
        default:
            c = ' ';
            break;
    }

    // Lowercase for black pieces
    if (color_ == Color::Black) {
        c = static_cast<char>(c + ('a' - 'A'));
    }

    return c;
}

Piece Piece::fromFenChar(char c) {
    Color pieceColor = (c >= 'A' && c <= 'Z') ? Color::White : Color::Black;
    char upper = (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c;

    PieceType pieceType;
    switch (upper) {
        case 'P':
            pieceType = PieceType::Pawn;
            break;
        case 'N':
            pieceType = PieceType::Knight;
            break;
        case 'B':
            pieceType = PieceType::Bishop;
            break;
        case 'R':
            pieceType = PieceType::Rook;
            break;
        case 'Q':
            pieceType = PieceType::Queen;
            break;
        case 'K':
            pieceType = PieceType::King;
            break;
        default:
            return Piece();  // Empty piece
    }

    return Piece(pieceType, pieceColor);
}

std::string Piece::toUnicode() const {
    if (isEmpty())
        return " ";

    // Unicode chess pieces: White (U+2654-U+2659), Black (U+265A-U+265F)
    const char* whitePieces[] = {"\u2659", "\u2658", "\u2657", "\u2656", "\u2655", "\u2654"};
    const char* blackPieces[] = {"\u265F", "\u265E", "\u265D", "\u265C", "\u265B", "\u265A"};

    int index = static_cast<int>(type_);
    return (color_ == Color::White) ? whitePieces[index] : blackPieces[index];
}

char Piece::toAscii() const {
    if (isEmpty())
        return '.';

    char c;
    switch (type_) {
        case PieceType::Pawn:
            c = 'P';
            break;
        case PieceType::Knight:
            c = 'N';
            break;
        case PieceType::Bishop:
            c = 'B';
            break;
        case PieceType::Rook:
            c = 'R';
            break;
        case PieceType::Queen:
            c = 'Q';
            break;
        case PieceType::King:
            c = 'K';
            break;
        default:
            c = '.';
            break;
    }

    // Lowercase for black pieces
    if (color_ == Color::Black) {
        c = static_cast<char>(c + ('a' - 'A'));
    }

    return c;
}

}  // namespace cchess
