#include "Notation.h"

#include "Board.h"
#include "Square.h"
#include "movegen/MoveGenerator.h"

namespace cchess {

static char pieceTypeToChar(PieceType pt) {
    switch (pt) {
        case PieceType::Knight: return 'N';
        case PieceType::Bishop: return 'B';
        case PieceType::Rook:   return 'R';
        case PieceType::Queen:  return 'Q';
        case PieceType::King:   return 'K';
        default:                return '?';
    }
}

static char promotionChar(PieceType pt) {
    switch (pt) {
        case PieceType::Queen:  return 'Q';
        case PieceType::Rook:   return 'R';
        case PieceType::Bishop: return 'B';
        case PieceType::Knight: return 'N';
        default:                return 'Q';
    }
}

std::string moveToSan(const Board& board, const Move& move) {
    if (move.isNull()) {
        return "--";
    }

    // Castling
    if (move.isCastling()) {
        bool kingside = getFile(move.to()) > getFile(move.from());
        std::string san = kingside ? "O-O" : "O-O-O";

        // Check/checkmate suffix: make the move on a copy
        Board copy = board;
        copy.makeMoveUnchecked(move);
        if (copy.isInCheck()) {
            san += copy.isCheckmate() ? "#" : "+";
        }
        return san;
    }

    const Position& pos = board.position();
    PieceType pt = pos.pieceAt(move.from()).type();
    Square from = move.from();
    Square to = move.to();
    bool capture = move.isCapture();

    std::string san;

    if (pt == PieceType::Pawn) {
        // Pawn moves: file prefix on captures
        if (capture) {
            san += fileToChar(getFile(from));
            san += 'x';
        }
        san += squareToString(to);

        // Promotion
        if (move.isPromotion()) {
            san += '=';
            san += promotionChar(move.promotion());
        }
    } else {
        // Piece letter
        san += pieceTypeToChar(pt);

        // Disambiguation: check other pieces of same type that can reach the same square
        MoveList legalMoves = board.getLegalMoves();
        bool needFile = false;
        bool needRank = false;
        bool ambiguous = false;

        for (const Move& other : legalMoves) {
            if (other.from() == from) continue;             // same move
            if (other.to() != to) continue;                 // different target
            if (pos.pieceAt(other.from()).type() != pt) continue;  // different piece type

            ambiguous = true;
            if (getFile(other.from()) == getFile(from)) {
                needRank = true;
            }
            if (getRank(other.from()) == getRank(from)) {
                needFile = true;
            }
        }

        // If ambiguous but neither same file nor same rank, file alone suffices
        if (ambiguous && !needFile && !needRank) {
            needFile = true;
        }

        if (needFile) san += fileToChar(getFile(from));
        if (needRank) san += rankToChar(getRank(from));

        if (capture) san += 'x';
        san += squareToString(to);
    }

    // Check/checkmate suffix
    Board copy = board;
    copy.makeMoveUnchecked(move);
    if (copy.isInCheck()) {
        san += copy.isCheckmate() ? "#" : "+";
    }

    return san;
}

}  // namespace cchess
