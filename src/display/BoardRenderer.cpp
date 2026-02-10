#include "BoardRenderer.h"

#include "../core/Square.h"

#include <sstream>

namespace cchess {

std::string BoardRenderer::render(const Board& board) {
    std::ostringstream oss;

    // Top coordinate labels
    oss << renderCoordinates() << "\n";

    // Render ranks from 8 down to 1
    for (int rank = 7; rank >= 0; --rank) {
        oss << (rank + 1) << " ";  // Rank number
        oss << renderRank(board, static_cast<Rank>(rank));
        oss << " " << (rank + 1);  // Rank number on right
        oss << "\n";
    }

    // Bottom coordinate labels
    oss << renderCoordinates();

    return oss.str();
}

std::string BoardRenderer::renderRank(const Board& board, Rank rank) {
    std::ostringstream oss;

    for (int file = 0; file < 8; ++file) {
        Square sq = makeSquare(static_cast<File>(file), rank);
        const Piece& piece = board.position().pieceAt(sq);
        oss << renderSquare(piece);

        // Add space between squares
        if (file < 7)
            oss << " ";
    }

    return oss.str();
}

std::string BoardRenderer::renderSquare(const Piece& piece) {
    if (piece.isEmpty()) {
        return ".";
    }
    return std::string(1, piece.toAscii());
}

std::string BoardRenderer::renderCoordinates() {
    return "  a b c d e f g h";
}

std::string BoardRenderer::renderPositionInfo(const Board& board) {
    std::ostringstream oss;

    oss << "\nPosition Information:\n";
    oss << "  FEN: " << board.toFen() << "\n";
    oss << "  Side to move: " << (board.sideToMove() == Color::White ? "White" : "Black") << "\n";

    // Castling rights
    auto rights = board.castlingRights();
    oss << "  Castling rights: ";
    if (rights == NO_CASTLING) {
        oss << "None";
    } else {
        if (rights & WHITE_KINGSIDE)
            oss << "K";
        if (rights & WHITE_QUEENSIDE)
            oss << "Q";
        if (rights & BLACK_KINGSIDE)
            oss << "k";
        if (rights & BLACK_QUEENSIDE)
            oss << "q";
    }
    oss << "\n";

    // En passant
    auto epSquare = board.enPassantSquare();
    oss << "  En passant: ";
    if (epSquare == SQUARE_NONE) {
        oss << "None";
    } else {
        oss << squareToString(epSquare);
    }
    oss << "\n";

    // Move counters
    oss << "  Halfmove clock: " << board.halfmoveClock() << "\n";
    oss << "  Fullmove number: " << board.fullmoveNumber() << "\n";

    return oss.str();
}

}  // namespace cchess
