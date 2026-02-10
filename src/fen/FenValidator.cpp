#include "FenValidator.h"

namespace cchess {

bool FenValidator::validate(const Position& position, std::string* error) {
    if (!validateKings(position, error))
        return false;
    if (!validatePawns(position, error))
        return false;
    if (!validateEnPassant(position, error))
        return false;
    return true;
}

bool FenValidator::validateKings(const Position& position, std::string* error) {
    int whiteKings = countPieces(position, PieceType::King, Color::White);
    int blackKings = countPieces(position, PieceType::King, Color::Black);

    if (whiteKings != 1) {
        if (error) {
            *error = "Position must have exactly 1 white king, found " + std::to_string(whiteKings);
        }
        return false;
    }

    if (blackKings != 1) {
        if (error) {
            *error = "Position must have exactly 1 black king, found " + std::to_string(blackKings);
        }
        return false;
    }

    return true;
}

bool FenValidator::validatePawns(const Position& position, std::string* error) {
    // Check that no pawns are on rank 1 or rank 8
    for (int file = 0; file < 8; ++file) {
        Square sq1 = makeSquare(static_cast<File>(file), RANK_1);
        Square sq8 = makeSquare(static_cast<File>(file), RANK_8);

        const Piece& piece1 = position.pieceAt(sq1);
        const Piece& piece8 = position.pieceAt(sq8);

        if (piece1.type() == PieceType::Pawn) {
            if (error) {
                *error = "Pawns cannot be on rank 1";
            }
            return false;
        }

        if (piece8.type() == PieceType::Pawn) {
            if (error) {
                *error = "Pawns cannot be on rank 8";
            }
            return false;
        }
    }

    return true;
}

bool FenValidator::validateEnPassant(const Position& position, std::string* error) {
    Square epSquare = position.enPassantSquare();
    if (epSquare == SQUARE_NONE) {
        return true;
    }

    Rank rank = getRank(epSquare);
    Color sideToMove = position.sideToMove();

    // En passant square should be on rank 3 (after white double push) or rank 6 (after black)
    if (sideToMove == Color::White && rank != RANK_6) {
        if (error) {
            *error = "When white is to move, en passant square must be on rank 6";
        }
        return false;
    }

    if (sideToMove == Color::Black && rank != RANK_3) {
        if (error) {
            *error = "When black is to move, en passant square must be on rank 3";
        }
        return false;
    }

    return true;
}

int FenValidator::countPieces(const Position& position, PieceType type, Color color) {
    int count = 0;
    for (Square sq = 0; sq < 64; ++sq) {
        const Piece& piece = position.pieceAt(sq);
        if (piece.type() == type && piece.color() == color) {
            ++count;
        }
    }
    return count;
}

}  // namespace cchess
