#include "MoveGenerator.h"

#include "AttackTables.h"

#include <algorithm>
#include <iterator>

namespace cchess {

// ============================================================================
// Helper Functions
// ============================================================================

Square MoveGenerator::findKingSquare(const Position& pos, Color color) {
    Bitboard kings = pos.pieces(PieceType::King, color);
    return kings ? lsb(kings) : SQUARE_NONE;
}

// ============================================================================
// Per-Piece Move Generators
// ============================================================================

void MoveGenerator::generatePawnMoves(const Position& pos, Square from, MoveList& moves) {
    Color us = pos.pieceAt(from).color();

    File fromFile = getFile(from);
    Rank fromRank = getRank(from);

    int direction = (us == Color::White) ? 1 : -1;
    Rank startRank = (us == Color::White) ? RANK_2 : RANK_7;
    Rank promotionRank = (us == Color::White) ? RANK_8 : RANK_1;

    // Forward one square
    int r = static_cast<int>(fromRank) + direction;
    if (r >= 0 && r <= 7) {
        Bitboard occupied = pos.occupied();
        Square to = makeSquare(fromFile, static_cast<Rank>(r));
        if (!testBit(occupied, to)) {
            if (static_cast<Rank>(r) == promotionRank) {
                moves.push_back(Move::makePromotion(from, to, PieceType::Queen));
                moves.push_back(Move::makePromotion(from, to, PieceType::Rook));
                moves.push_back(Move::makePromotion(from, to, PieceType::Bishop));
                moves.push_back(Move::makePromotion(from, to, PieceType::Knight));
            } else {
                moves.push_back(Move(from, to, MoveType::Normal));

                // Forward two squares from starting position
                if (fromRank == startRank) {
                    int r2 = r + direction;
                    Square doubleTo = makeSquare(fromFile, static_cast<Rank>(r2));
                    if (!testBit(occupied, doubleTo)) {
                        moves.push_back(Move(from, doubleTo, MoveType::Normal));
                    }
                }
            }
        }
    }

    // Captures (diagonal)
    int captureRank = static_cast<int>(fromRank) + direction;
    if (captureRank >= 0 && captureRank <= 7) {
        Bitboard enemies = pos.pieces(~us);
        for (int df : {-1, 1}) {
            int f = static_cast<int>(fromFile) + df;
            if (f >= 0 && f <= 7) {
                Square to = makeSquare(static_cast<File>(f), static_cast<Rank>(captureRank));

                // Regular capture
                if (testBit(enemies, to)) {
                    if (static_cast<Rank>(captureRank) == promotionRank) {
                        moves.push_back(Move::makePromotionCapture(from, to, PieceType::Queen));
                        moves.push_back(Move::makePromotionCapture(from, to, PieceType::Rook));
                        moves.push_back(Move::makePromotionCapture(from, to, PieceType::Bishop));
                        moves.push_back(Move::makePromotionCapture(from, to, PieceType::Knight));
                    } else {
                        moves.push_back(Move(from, to, MoveType::Capture));
                    }
                }

                // En passant capture
                if (to == pos.enPassantSquare()) {
                    moves.push_back(Move::makeEnPassant(from, to));
                }
            }
        }
    }
}

void MoveGenerator::generateKnightMoves(const Position& pos, Square from,
                                        MoveList& moves) {
    Bitboard friends = pos.pieces(pos.pieceAt(from).color());
    Bitboard targets = KNIGHT_ATTACKS[from] & ~friends;
    Bitboard occupied = pos.occupied();

    while (targets) {
        Square to = popLsb(targets);
        moves.push_back(Move(from, to, testBit(occupied, to) ? MoveType::Capture : MoveType::Normal));
    }
}

void MoveGenerator::generateBishopMoves(const Position& pos, Square from,
                                        MoveList& moves) {
    Bitboard friends = pos.pieces(pos.pieceAt(from).color());
    Bitboard occupied = pos.occupied();
    Bitboard targets = bishopAttacks(from, occupied) & ~friends;
    while (targets) {
        Square to = popLsb(targets);
        moves.push_back(Move(from, to, testBit(occupied, to) ? MoveType::Capture : MoveType::Normal));
    }
}

void MoveGenerator::generateRookMoves(const Position& pos, Square from, MoveList& moves) {
    Bitboard friends = pos.pieces(pos.pieceAt(from).color());
    Bitboard occupied = pos.occupied();
    Bitboard targets = rookAttacks(from, occupied) & ~friends;
    while (targets) {
        Square to = popLsb(targets);
        moves.push_back(Move(from, to, testBit(occupied, to) ? MoveType::Capture : MoveType::Normal));
    }
}

void MoveGenerator::generateQueenMoves(const Position& pos, Square from, MoveList& moves) {
    Bitboard friends = pos.pieces(pos.pieceAt(from).color());
    Bitboard occupied = pos.occupied();
    Bitboard targets = (rookAttacks(from, occupied) | bishopAttacks(from, occupied)) & ~friends;
    while (targets) {
        Square to = popLsb(targets);
        moves.push_back(Move(from, to, testBit(occupied, to) ? MoveType::Capture : MoveType::Normal));
    }
}

void MoveGenerator::generateKingMoves(const Position& pos, Square from, MoveList& moves) {
    Bitboard friends = pos.pieces(pos.pieceAt(from).color());
    Bitboard targets = KING_ATTACKS[from] & ~friends;
    Bitboard occupied = pos.occupied();

    while (targets) {
        Square to = popLsb(targets);
        moves.push_back(Move(from, to, testBit(occupied, to) ? MoveType::Capture : MoveType::Normal));
    }
}

void MoveGenerator::generateCastlingMoves(const Position& pos, MoveList& moves) {
    Color us = pos.sideToMove();
    Square kingSquare = findKingSquare(pos, us);

    if (kingSquare == SQUARE_NONE) {
        return;  // No king found (invalid position)
    }

    // Can't castle if in check
    if (isInCheck(pos, us)) {
        return;
    }

    Bitboard occupied = pos.occupied();
    Rank rank = (us == Color::White) ? RANK_1 : RANK_8;

    // Kingside castling
    CastlingRights kingsideRight = (us == Color::White) ? WHITE_KINGSIDE : BLACK_KINGSIDE;
    if ((pos.castlingRights() & kingsideRight) != 0) {
        Square f1 = makeSquare(FILE_F, rank);
        Square g1 = makeSquare(FILE_G, rank);

        // Check if squares between king and rook are empty
        if (!testBit(occupied, f1) && !testBit(occupied, g1)) {
            // Check if king passes through or lands on attacked square
            if (!isSquareAttacked(pos, f1, ~us) && !isSquareAttacked(pos, g1, ~us)) {
                moves.push_back(Move::makeCastling(kingSquare, g1));
            }
        }
    }

    // Queenside castling
    CastlingRights queensideRight = (us == Color::White) ? WHITE_QUEENSIDE : BLACK_QUEENSIDE;
    if ((pos.castlingRights() & queensideRight) != 0) {
        Square d1 = makeSquare(FILE_D, rank);
        Square c1 = makeSquare(FILE_C, rank);
        Square b1 = makeSquare(FILE_B, rank);

        // Check if squares between king and rook are empty
        if (!testBit(occupied, d1) && !testBit(occupied, c1) && !testBit(occupied, b1)) {
            // Check if king passes through or lands on attacked square
            if (!isSquareAttacked(pos, d1, ~us) && !isSquareAttacked(pos, c1, ~us)) {
                moves.push_back(Move::makeCastling(kingSquare, c1));
            }
        }
    }
}

// ============================================================================
// Pseudo-Legal Move Generation
// ============================================================================

MoveList MoveGenerator::generatePseudoLegalMoves(const Position& pos) {
    MoveList moves;

    Color us = pos.sideToMove();

    // Iterate only our pieces using bitboard
    Bitboard ourPieces = pos.pieces(us);
    while (ourPieces) {
        Square sq = popLsb(ourPieces);

        switch (pos.pieceAt(sq).type()) {
            case PieceType::Pawn:
                generatePawnMoves(pos, sq, moves);
                break;
            case PieceType::Knight:
                generateKnightMoves(pos, sq, moves);
                break;
            case PieceType::Bishop:
                generateBishopMoves(pos, sq, moves);
                break;
            case PieceType::Rook:
                generateRookMoves(pos, sq, moves);
                break;
            case PieceType::Queen:
                generateQueenMoves(pos, sq, moves);
                break;
            case PieceType::King:
                generateKingMoves(pos, sq, moves);
                break;
            default:
                break;
        }
    }

    // Add castling moves
    generateCastlingMoves(pos, moves);

    return moves;
}

// ============================================================================
// Check Detection
// ============================================================================

bool MoveGenerator::isSquareAttacked(const Position& pos, Square sq, Color byColor) {
    // Knight attacks (precomputed table lookup)
    if (KNIGHT_ATTACKS[sq] & pos.pieces(PieceType::Knight, byColor))
        return true;

    // King attacks (precomputed table lookup)
    if (KING_ATTACKS[sq] & pos.pieces(PieceType::King, byColor))
        return true;

    // Pawn attacks (bitboard shift)
    Bitboard pawns = pos.pieces(PieceType::Pawn, byColor);
    if (pawns) {
        Bitboard sqBB = squareBB(sq);
        if (byColor == Color::White) {
            if ((shiftNorthEast(pawns) | shiftNorthWest(pawns)) & sqBB)
                return true;
        } else {
            if ((shiftSouthEast(pawns) | shiftSouthWest(pawns)) & sqBB)
                return true;
        }
    }

    Bitboard occupied = pos.occupied();

    // Bishop/Queen attacks (magic bitboard lookup)
    Bitboard bishopsQueens =
        pos.pieces(PieceType::Bishop, byColor) | pos.pieces(PieceType::Queen, byColor);
    if (bishopAttacks(sq, occupied) & bishopsQueens)
        return true;

    // Rook/Queen attacks (magic bitboard lookup)
    Bitboard rooksQueens =
        pos.pieces(PieceType::Rook, byColor) | pos.pieces(PieceType::Queen, byColor);
    if (rookAttacks(sq, occupied) & rooksQueens)
        return true;

    return false;
}

bool MoveGenerator::isInCheck(const Position& pos, Color side) {
    Square kingSquare = findKingSquare(pos, side);
    if (kingSquare == SQUARE_NONE) {
        return false;  // No king (invalid position)
    }
    return isSquareAttacked(pos, kingSquare, ~side);
}

bool MoveGenerator::moveLeavesKingInCheck(Position& pos, const Move& move) {
    Color us = pos.sideToMove();
    UndoInfo undo = pos.makeMove(move);
    bool inCheck = isInCheck(pos, us);
    pos.unmakeMove(move, undo);
    return inCheck;
}

// ============================================================================
// Legal Move Generation
// ============================================================================

MoveList MoveGenerator::generateLegalMoves(const Position& pos) {
    MoveList pseudoLegal = generatePseudoLegalMoves(pos);
    MoveList legal;

    Position workspace = pos;
    std::copy_if(pseudoLegal.begin(), pseudoLegal.end(), std::back_inserter(legal),
        [&workspace](const Move& move) {
            return !moveLeavesKingInCheck(workspace, move);
        });

    return legal;
}

bool MoveGenerator::isLegal(const Position& pos, const Move& move) {
    // First check if the move is pseudo-legal
    MoveList pseudoLegal = generatePseudoLegalMoves(pos);
    bool found = std::any_of(pseudoLegal.begin(), pseudoLegal.end(),
        [&move](const Move& m) { return m == move; });

    if (!found) {
        return false;
    }

    // Then check if it leaves the king in check
    Position workspace = pos;
    return !moveLeavesKingInCheck(workspace, move);
}

// ============================================================================
// Game State Queries
// ============================================================================

bool MoveGenerator::isCheckmate(const Position& pos) {
    Color us = pos.sideToMove();
    return isInCheck(pos, us) && generateLegalMoves(pos).empty();
}

bool MoveGenerator::isStalemate(const Position& pos) {
    Color us = pos.sideToMove();
    return !isInCheck(pos, us) && generateLegalMoves(pos).empty();
}

bool MoveGenerator::isDraw(const Position& pos) {
    // 50-move rule (halfmove clock >= 100 means 50 full moves)
    return pos.halfmoveClock() >= 100;
}

}  // namespace cchess
