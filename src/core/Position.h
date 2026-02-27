#ifndef CCHESS_POSITION_H
#define CCHESS_POSITION_H

#include "Bitboard.h"
#include "Move.h"
#include "Piece.h"
#include "Types.h"
#include "ai/PST.h"

#include <array>
#include <cassert>
#include <cstdint>

namespace cchess {

struct UndoInfo {
    Piece capturedPiece;
    CastlingRights castlingRights;
    Square enPassantSquare;
    int halfmoveClock;
    uint64_t hash;
};

class Position {
public:
    Position();

    // Piece access
    const Piece& pieceAt(Square sq) const {
        assert(cchess::squareIsValid(sq));
        return board_[sq];
    }
    void setPiece(Square sq, const Piece& piece);
    void clearSquare(Square sq);

    uint64_t hash() const { return hash_; }
    void computeHash();

    eval::Score psqt() const { return psqt_; }

    // Game state getters
    Color sideToMove() const { return sideToMove_; }
    CastlingRights castlingRights() const { return castlingRights_; }
    Square enPassantSquare() const { return enPassantSquare_; }
    int halfmoveClock() const { return halfmoveClock_; }
    int fullmoveNumber() const { return fullmoveNumber_; }

    // Game state setters
    void setSideToMove(Color color) { sideToMove_ = color; }
    void setCastlingRights(CastlingRights rights) { castlingRights_ = rights; }
    void setEnPassantSquare(Square sq) { enPassantSquare_ = sq; }
    void setHalfmoveClock(int clock) { halfmoveClock_ = clock; }
    void setFullmoveNumber(int number) { fullmoveNumber_ = number; }

    // Move execution
    UndoInfo makeMove(const Move& move);
    void unmakeMove(const Move& move, const UndoInfo& undo);

    // Null move (flip side, clear en passant, update hash)
    void makeNullMove();
    void unmakeNullMove(Square prevEp, uint64_t prevHash);

    // Board operations
    void clear();

    // Bitboard accessors
    Bitboard pieces(PieceType pt) const { return pieceBB_[static_cast<int>(pt)]; }
    Bitboard pieces(Color c) const { return colorBB_[static_cast<int>(c)]; }
    Bitboard pieces(PieceType pt, Color c) const { return pieces(pt) & pieces(c); }
    Bitboard occupied() const { return occupied_; }

    // Cached king squares (updated incrementally)
    Square kingSquare(Color c) const { return kingSquare_[static_cast<int>(c)]; }

    // Game state update methods
    void incrementHalfmoveClock() { ++halfmoveClock_; }
    void resetHalfmoveClock() { halfmoveClock_ = 0; }
    void incrementFullmoveNumber() { ++fullmoveNumber_; }
    void flipSideToMove() { sideToMove_ = ~sideToMove_; }
    void removeCastlingRights(CastlingRights rights) {
        castlingRights_ = castlingRights_ & ~rights;
    }

private:
    void updateCastlingRightsForMove(const Move& move, const Piece& movedPiece);

    // Direct bitboard manipulation for makeMove/unmakeMove hot path.
    // These use XOR and assume correct preconditions (no redundant checks).
    void movePieceBB(Square from, Square to, PieceType pt, Color c) {
        Bitboard fromTo = squareBB(from) | squareBB(to);
        pieceBB_[static_cast<int>(pt)] ^= fromTo;
        colorBB_[static_cast<int>(c)] ^= fromTo;
        board_[to] = board_[from];
        board_[from] = Piece();
        psqt_ -= eval::pstValue(pt, c, from);
        psqt_ += eval::pstValue(pt, c, to);
    }

    void removePieceBB(Square sq, PieceType pt, Color c) {
        pieceBB_[static_cast<int>(pt)] ^= squareBB(sq);
        colorBB_[static_cast<int>(c)] ^= squareBB(sq);
        board_[sq] = Piece();
        psqt_ -= eval::pstValue(pt, c, sq);
    }

    void putPieceBB(Square sq, PieceType pt, Color c) {
        pieceBB_[static_cast<int>(pt)] ^= squareBB(sq);
        colorBB_[static_cast<int>(c)] ^= squareBB(sq);
        board_[sq] = Piece(pt, c);
        psqt_ += eval::pstValue(pt, c, sq);
    }

    void updateOccupied() { occupied_ = colorBB_[0] | colorBB_[1]; }

    std::array<Piece, 64> board_;

    // Bitboards (synchronized with board_ array)
    std::array<Bitboard, 6> pieceBB_;  // Indexed by PieceType (Pawn..King)
    std::array<Bitboard, 2> colorBB_;  // Indexed by Color (White, Black)
    Bitboard occupied_;
    std::array<Square, 2> kingSquare_;  // Indexed by Color

    eval::Score psqt_;
    Color sideToMove_;
    CastlingRights castlingRights_;
    Square enPassantSquare_;
    int halfmoveClock_;
    int fullmoveNumber_;
    uint64_t hash_ = 0;
};

}  // namespace cchess

#endif  // CCHESS_POSITION_H
