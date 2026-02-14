#ifndef CCHESS_BOARD_H
#define CCHESS_BOARD_H

#include "Move.h"
#include "MoveList.h"
#include "Position.h"

#include <optional>
#include <string>

namespace cchess {

class Board {
public:
    // Constructors
    Board();  // Default: starting position
    explicit Board(const std::string& fen);

    // FEN operations
    void setFromFen(const std::string& fen);
    std::string toFen() const;

    // Position access
    const Position& position() const { return position_; }
    Position& position() { return position_; }

    // Piece access
    const Piece& at(Square sq) const;
    const Piece& at(const std::string& algebraic) const;

    // Board manipulation
    void clear();
    void addPiece(const Piece& piece, const std::string& algebraic);

    // Game state
    Color sideToMove() const { return position_.sideToMove(); }
    CastlingRights castlingRights() const { return position_.castlingRights(); }
    Square enPassantSquare() const { return position_.enPassantSquare(); }
    int halfmoveClock() const { return position_.halfmoveClock(); }
    int fullmoveNumber() const { return position_.fullmoveNumber(); }

    // Move operations
    bool makeMove(const Move& move);
    UndoInfo makeMoveUnchecked(const Move& move);
    void unmakeMove(const Move& move, const UndoInfo& undo);
    MoveList getLegalMoves() const;
    bool isMoveLegal(const Move& move) const;
    std::optional<Move> findLegalMove(Square from, Square to,
                                      PieceType promotion = PieceType::None) const;

    // Game state queries
    bool isInCheck() const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isDraw() const;

    // Standard starting position FEN
    static constexpr const char* STARTING_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
    Position position_;
};

}  // namespace cchess

#endif  // CCHESS_BOARD_H
