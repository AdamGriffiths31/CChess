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
    UndoInfo undo;
    undo.castlingRights = castlingRights_;
    undo.enPassantSquare = enPassantSquare_;
    undo.halfmoveClock = halfmoveClock_;

    const Square from = move.from();
    const Square to = move.to();
    const Color us = sideToMove_;
    const Color them = ~us;
    const Piece movedPiece = board_[from];
    const PieceType pt = movedPiece.type();

    undo.capturedPiece = board_[to];

    if (move.isCastling()) {
        movePieceBB(from, to, PieceType::King, us);

        Rank rank = getRank(from);
        Square rookFrom, rookTo;
        if (getFile(to) == FILE_G) {
            rookFrom = makeSquare(FILE_H, rank);
            rookTo = makeSquare(FILE_F, rank);
        } else {
            rookFrom = makeSquare(FILE_A, rank);
            rookTo = makeSquare(FILE_D, rank);
        }
        movePieceBB(rookFrom, rookTo, PieceType::Rook, us);
    } else if (move.isEnPassant()) {
        movePieceBB(from, to, PieceType::Pawn, us);

        int direction = (us == Color::White) ? -1 : 1;
        Rank captureRank = static_cast<Rank>(getRank(to) + direction);
        Square capturedSq = makeSquare(getFile(to), captureRank);
        undo.capturedPiece = board_[capturedSq];
        removePieceBB(capturedSq, PieceType::Pawn, them);
    } else {
        if (move.isCapture()) {
            removePieceBB(to, undo.capturedPiece.type(), them);
        }

        if (move.isPromotion()) {
            removePieceBB(from, PieceType::Pawn, us);
            putPieceBB(to, move.promotion(), us);
        } else {
            movePieceBB(from, to, pt, us);
        }
    }

    updateOccupied();

    // Update halfmove clock
    if (pt == PieceType::Pawn || move.isCapture()) {
        halfmoveClock_ = 0;
    } else {
        ++halfmoveClock_;
    }

    // Update fullmove number
    if (us == Color::Black) {
        ++fullmoveNumber_;
    }

    // Update castling rights
    updateCastlingRightsForMove(move, movedPiece);

    // Update en passant square
    if (pt == PieceType::Pawn) {
        int rankDiff =
            std::abs(static_cast<int>(getRank(to)) - static_cast<int>(getRank(from)));
        if (rankDiff == 2) {
            int dir = (us == Color::White) ? 1 : -1;
            Rank epRank = static_cast<Rank>(getRank(from) + dir);
            enPassantSquare_ = makeSquare(getFile(from), epRank);
        } else {
            enPassantSquare_ = SQUARE_NONE;
        }
    } else {
        enPassantSquare_ = SQUARE_NONE;
    }

    sideToMove_ = ~sideToMove_;

    return undo;
}

void Position::unmakeMove(const Move& move, const UndoInfo& undo) {
    sideToMove_ = ~sideToMove_;

    const Square from = move.from();
    const Square to = move.to();
    const Color us = sideToMove_;
    const Color them = ~us;

    if (move.isCastling()) {
        movePieceBB(to, from, PieceType::King, us);

        Rank rank = getRank(from);
        Square rookFrom, rookTo;
        if (getFile(to) == FILE_G) {
            rookFrom = makeSquare(FILE_H, rank);
            rookTo = makeSquare(FILE_F, rank);
        } else {
            rookFrom = makeSquare(FILE_A, rank);
            rookTo = makeSquare(FILE_D, rank);
        }
        movePieceBB(rookTo, rookFrom, PieceType::Rook, us);
    } else if (move.isEnPassant()) {
        movePieceBB(to, from, PieceType::Pawn, us);

        int direction = (us == Color::White) ? -1 : 1;
        Rank captureRank = static_cast<Rank>(getRank(to) + direction);
        Square capturedSq = makeSquare(getFile(to), captureRank);
        putPieceBB(capturedSq, PieceType::Pawn, them);
    } else {
        if (move.isPromotion()) {
            removePieceBB(to, move.promotion(), us);
            putPieceBB(from, PieceType::Pawn, us);
        } else {
            movePieceBB(to, from, board_[to].type(), us);
        }

        if (undo.capturedPiece.type() != PieceType::None) {
            putPieceBB(to, undo.capturedPiece.type(), them);
        }
    }

    updateOccupied();

    castlingRights_ = undo.castlingRights;
    enPassantSquare_ = undo.enPassantSquare;
    halfmoveClock_ = undo.halfmoveClock;

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
