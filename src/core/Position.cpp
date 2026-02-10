#include "Position.h"

#include "Square.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace cchess {

Position::Position()
    : pieceBB_{},
      colorBB_{},
      occupied_(BB_EMPTY),
      sideToMove_(Color::White),
      castlingRights_(NO_CASTLING),
      enPassantSquare_(SQUARE_NONE),
      halfmoveClock_(0),
      fullmoveNumber_(1) {
    clear();
}

const Piece& Position::pieceAt(Square sq) const {
    assert(cchess::squareIsValid(sq));
    return board_[sq];
}

void Position::setPiece(Square sq, const Piece& piece) {
    assert(cchess::squareIsValid(sq));
    assert(piece.isValid());

    // Clear old piece from bitboards
    const Piece& old = board_[sq];
    if (!old.isEmpty()) {
        pieceBB_[static_cast<int>(old.type())] &= ~squareBB(sq);
        colorBB_[static_cast<int>(old.color())] &= ~squareBB(sq);
    }

    // Update array
    board_[sq] = piece;

    // Set new piece in bitboards
    if (!piece.isEmpty()) {
        pieceBB_[static_cast<int>(piece.type())] |= squareBB(sq);
        colorBB_[static_cast<int>(piece.color())] |= squareBB(sq);
    }

    occupied_ = colorBB_[0] | colorBB_[1];
}

void Position::clearSquare(Square sq) {
    assert(cchess::squareIsValid(sq));

    const Piece& old = board_[sq];
    if (!old.isEmpty()) {
        pieceBB_[static_cast<int>(old.type())] &= ~squareBB(sq);
        colorBB_[static_cast<int>(old.color())] &= ~squareBB(sq);
        occupied_ &= ~squareBB(sq);
    }

    board_[sq] = Piece();
}

void Position::clear() {
    std::generate(board_.begin(), board_.end(), []() { return Piece(); });
    pieceBB_.fill(BB_EMPTY);
    colorBB_.fill(BB_EMPTY);
    occupied_ = BB_EMPTY;
}

// ============================================================================
// Move Execution
// ============================================================================

UndoInfo Position::makeMove(const Move& move) {
    // Save undo state
    UndoInfo undo;
    undo.capturedPiece = pieceAt(move.to());
    undo.castlingRights = castlingRights_;
    undo.enPassantSquare = enPassantSquare_;
    undo.halfmoveClock = halfmoveClock_;

    Piece movedPiece = pieceAt(move.from());

    // Execute piece movement by move type
    if (move.isCastling()) {
        Square kingFrom = move.from();
        Square kingTo = move.to();

        // Move the king
        setPiece(kingTo, movedPiece);
        clearSquare(kingFrom);

        // Determine and move the rook
        Square rookFrom, rookTo;
        if (getFile(kingTo) == FILE_G) {
            Rank rank = getRank(kingFrom);
            rookFrom = makeSquare(FILE_H, rank);
            rookTo = makeSquare(FILE_F, rank);
        } else {
            Rank rank = getRank(kingFrom);
            rookFrom = makeSquare(FILE_A, rank);
            rookTo = makeSquare(FILE_D, rank);
        }
        Piece rook = pieceAt(rookFrom);
        setPiece(rookTo, rook);
        clearSquare(rookFrom);
    } else if (move.isEnPassant()) {
        // Move the pawn
        setPiece(move.to(), movedPiece);
        clearSquare(move.from());

        // Remove the captured pawn
        int direction = (sideToMove_ == Color::White) ? -1 : 1;
        Rank captureRank = static_cast<Rank>(getRank(move.to()) + direction);
        Square capturedPawnSq = makeSquare(getFile(move.to()), captureRank);
        // Save the actual captured pawn in undo (it's not on move.to())
        undo.capturedPiece = pieceAt(capturedPawnSq);
        clearSquare(capturedPawnSq);
    } else {
        // Regular move or capture
        setPiece(move.to(), movedPiece);
        clearSquare(move.from());

        // Handle promotion
        if (move.isPromotion()) {
            setPiece(move.to(), Piece(move.promotion(), sideToMove_));
        }
    }

    // Update halfmove clock
    if (movedPiece.type() == PieceType::Pawn || move.isCapture()) {
        halfmoveClock_ = 0;
    } else {
        ++halfmoveClock_;
    }

    // Update fullmove number
    if (sideToMove_ == Color::Black) {
        ++fullmoveNumber_;
    }

    // Update castling rights
    updateCastlingRightsForMove(move, movedPiece);

    // Update en passant square
    if (movedPiece.type() == PieceType::Pawn) {
        int rankDiff =
            std::abs(static_cast<int>(getRank(move.to())) - static_cast<int>(getRank(move.from())));
        if (rankDiff == 2) {
            int dir = (sideToMove_ == Color::White) ? 1 : -1;
            Rank epRank = static_cast<Rank>(getRank(move.from()) + dir);
            enPassantSquare_ = makeSquare(getFile(move.from()), epRank);
        } else {
            enPassantSquare_ = SQUARE_NONE;
        }
    } else {
        enPassantSquare_ = SQUARE_NONE;
    }

    // Flip side to move
    sideToMove_ = ~sideToMove_;

    return undo;
}

void Position::unmakeMove(const Move& move, const UndoInfo& undo) {
    // Flip side back
    sideToMove_ = ~sideToMove_;

    Piece movedPiece = pieceAt(move.to());

    // For promotions, the moved piece was originally a pawn
    if (move.isPromotion()) {
        movedPiece = Piece(PieceType::Pawn, sideToMove_);
    }

    // Reverse piece movement by move type
    if (move.isCastling()) {
        // Unmove the king
        setPiece(move.from(), movedPiece);
        clearSquare(move.to());

        // Unmove the rook
        Square rookFrom, rookTo;
        if (getFile(move.to()) == FILE_G) {
            Rank rank = getRank(move.from());
            rookFrom = makeSquare(FILE_H, rank);
            rookTo = makeSquare(FILE_F, rank);
        } else {
            Rank rank = getRank(move.from());
            rookFrom = makeSquare(FILE_A, rank);
            rookTo = makeSquare(FILE_D, rank);
        }
        Piece rook = pieceAt(rookTo);
        setPiece(rookFrom, rook);
        clearSquare(rookTo);
    } else if (move.isEnPassant()) {
        // Restore the moving pawn
        setPiece(move.from(), movedPiece);
        clearSquare(move.to());

        // Restore the captured pawn
        int direction = (sideToMove_ == Color::White) ? -1 : 1;
        Rank captureRank = static_cast<Rank>(getRank(move.to()) + direction);
        Square capturedPawnSq = makeSquare(getFile(move.to()), captureRank);
        setPiece(capturedPawnSq, undo.capturedPiece);
    } else {
        // Restore moving piece to source
        setPiece(move.from(), movedPiece);

        // Restore captured piece (or clear if no capture)
        if (undo.capturedPiece.type() != PieceType::None) {
            setPiece(move.to(), undo.capturedPiece);
        } else {
            clearSquare(move.to());
        }
    }

    // Restore game state
    castlingRights_ = undo.castlingRights;
    enPassantSquare_ = undo.enPassantSquare;
    halfmoveClock_ = undo.halfmoveClock;

    // Decrement fullmove if black moved
    if (sideToMove_ == Color::Black) {
        --fullmoveNumber_;
    }
}

void Position::updateCastlingRightsForMove(const Move& move, const Piece& movedPiece) {
    Color us = sideToMove_;

    // Remove castling rights if king moves
    if (movedPiece.type() == PieceType::King) {
        CastlingRights rights = (us == Color::White) ? WHITE_CASTLING : BLACK_CASTLING;
        removeCastlingRights(rights);
    }

    // Remove castling rights if rook moves
    if (movedPiece.type() == PieceType::Rook) {
        if (us == Color::White) {
            if (move.from() == makeSquare(FILE_A, RANK_1)) {
                removeCastlingRights(WHITE_QUEENSIDE);
            } else if (move.from() == makeSquare(FILE_H, RANK_1)) {
                removeCastlingRights(WHITE_KINGSIDE);
            }
        } else {
            if (move.from() == makeSquare(FILE_A, RANK_8)) {
                removeCastlingRights(BLACK_QUEENSIDE);
            } else if (move.from() == makeSquare(FILE_H, RANK_8)) {
                removeCastlingRights(BLACK_KINGSIDE);
            }
        }
    }

    // Remove castling rights if rook is captured
    if (move.isCapture()) {
        Color them = ~us;
        if (them == Color::White) {
            if (move.to() == makeSquare(FILE_A, RANK_1)) {
                removeCastlingRights(WHITE_QUEENSIDE);
            } else if (move.to() == makeSquare(FILE_H, RANK_1)) {
                removeCastlingRights(WHITE_KINGSIDE);
            }
        } else {
            if (move.to() == makeSquare(FILE_A, RANK_8)) {
                removeCastlingRights(BLACK_QUEENSIDE);
            } else if (move.to() == makeSquare(FILE_H, RANK_8)) {
                removeCastlingRights(BLACK_KINGSIDE);
            }
        }
    }
}

}  // namespace cchess
