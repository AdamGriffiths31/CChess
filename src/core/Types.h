#ifndef CCHESS_TYPES_H
#define CCHESS_TYPES_H

#include <cstdint>

namespace cchess {

// Color enumeration
enum class Color : uint8_t { White = 0, Black = 1, None = 2 };

// Piece type enumeration
enum class PieceType : uint8_t {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
    None = 6
};

// Square representation (0-63, where 0 = a1, 7 = h1, 56 = a8, 63 = h8)
using Square = uint8_t;

// Special square constants
constexpr Square SQUARE_NONE = 64;
constexpr Square SQUARE_A1 = 0;
constexpr Square SQUARE_H1 = 7;
constexpr Square SQUARE_A8 = 56;
constexpr Square SQUARE_H8 = 63;

// Castling rights bitfield
enum CastlingRights : uint8_t {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8,
    WHITE_CASTLING = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_CASTLING = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

// File and Rank types
using File = uint8_t;
using Rank = uint8_t;

constexpr File FILE_A = 0;
constexpr File FILE_B = 1;
constexpr File FILE_C = 2;
constexpr File FILE_D = 3;
constexpr File FILE_E = 4;
constexpr File FILE_F = 5;
constexpr File FILE_G = 6;
constexpr File FILE_H = 7;

constexpr Rank RANK_1 = 0;
constexpr Rank RANK_2 = 1;
constexpr Rank RANK_3 = 2;
constexpr Rank RANK_4 = 3;
constexpr Rank RANK_5 = 4;
constexpr Rank RANK_6 = 5;
constexpr Rank RANK_7 = 6;
constexpr Rank RANK_8 = 7;

// Utility functions for Color
inline Color operator~(Color c) {
    return Color(static_cast<uint8_t>(c) ^ 1);
}

inline bool colorIsValid(Color c) {
    return c == Color::White || c == Color::Black;
}

// Utility functions for PieceType
inline bool pieceTypeIsValid(PieceType pt) {
    return pt >= PieceType::Pawn && pt <= PieceType::King;
}

// Utility functions for Square
inline bool squareIsValid(Square sq) {
    return sq <= SQUARE_H8;  // No need to check >= 0 since Square is unsigned
}

inline File getFile(Square sq) {
    return sq & 7;
}

inline Rank getRank(Square sq) {
    return sq >> 3;
}

constexpr inline Square makeSquare(File f, Rank r) {
    return (r << 3) | f;
}

// Castling rights utilities
inline CastlingRights operator|(CastlingRights a, CastlingRights b) {
    return static_cast<CastlingRights>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline CastlingRights operator&(CastlingRights a, CastlingRights b) {
    return static_cast<CastlingRights>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline CastlingRights& operator|=(CastlingRights& a, CastlingRights b) {
    return a = a | b;
}

inline CastlingRights& operator&=(CastlingRights& a, CastlingRights b) {
    return a = a & b;
}

inline CastlingRights operator~(CastlingRights a) {
    return static_cast<CastlingRights>(~static_cast<uint8_t>(a) & 15);
}

}  // namespace cchess

#endif  // CCHESS_TYPES_H
