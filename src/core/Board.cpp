#include "Board.h"

#include "../fen/FenParser.h"
#include "../fen/FenValidator.h"
#include "../utils/Error.h"
#include "movegen/MoveGenerator.h"
#include "Square.h"

namespace cchess {

Board::Board() {
    setFromFen(STARTING_FEN);
}

Board::Board(const std::string& fen) {
    setFromFen(fen);
}

void Board::setFromFen(const std::string& fen) {
    position_ = FenParser::parse(fen);

    std::string error;
    if (!FenValidator::validate(position_, &error)) {
        throw FenValidationError(error);
    }
}

std::string Board::toFen() const {
    return FenParser::serialize(position_);
}

const Piece& Board::at(Square sq) const {
    if (!squareIsValid(sq)) {
        throw ChessError("Invalid square: " + std::to_string(sq));
    }
    return position_.pieceAt(sq);
}

const Piece& Board::at(const std::string& algebraic) const {
    auto sq = stringToSquare(algebraic);
    if (!sq) {
        throw ChessError("Invalid algebraic notation: " + algebraic);
    }
    return at(*sq);
}

void Board::clear() {
    position_.clear();
}

void Board::addPiece(const Piece& piece, const std::string& algebraic) {
    auto sq = stringToSquare(algebraic);
    if (!sq) {
        throw ChessError("Invalid algebraic notation: " + algebraic);
    }
    position_.setPiece(*sq, piece);
}

// ============================================================================
// Move Operations
// ============================================================================

bool Board::makeMove(const Move& move) {
    if (!MoveGenerator::isLegal(position_, move)) {
        return false;
    }
    position_.makeMove(move);
    return true;
}

UndoInfo Board::makeMoveUnchecked(const Move& move) {
    return position_.makeMove(move);
}

void Board::unmakeMove(const Move& move, const UndoInfo& undo) {
    position_.unmakeMove(move, undo);
}

MoveList Board::getLegalMoves() const {
    return MoveGenerator::generateLegalMoves(position_);
}

bool Board::isMoveLegal(const Move& move) const {
    return MoveGenerator::isLegal(position_, move);
}

// ============================================================================
// Game State Queries
// ============================================================================

bool Board::isInCheck() const {
    return MoveGenerator::isInCheck(position_, position_.sideToMove());
}

bool Board::isCheckmate() const {
    return MoveGenerator::isCheckmate(position_);
}

bool Board::isStalemate() const {
    return MoveGenerator::isStalemate(position_);
}

bool Board::isDraw() const {
    return MoveGenerator::isDraw(position_);
}

}  // namespace cchess
