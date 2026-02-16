#include "MoveGenerator.h"

#include "AttackTables.h"

#include <algorithm>
#include <iterator>

namespace cchess {

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

Bitboard MoveGenerator::pieceAttacks(PieceType pt, Square sq, Bitboard occupied) {
    switch (pt) {
        case PieceType::Knight:
            return KNIGHT_ATTACKS[sq];
        case PieceType::Bishop:
            return bishopAttacks(sq, occupied);
        case PieceType::Rook:
            return rookAttacks(sq, occupied);
        case PieceType::Queen:
            return rookAttacks(sq, occupied) | bishopAttacks(sq, occupied);
        case PieceType::King:
            return KING_ATTACKS[sq];
        default:
            return 0;
    }
}

void MoveGenerator::serializeMoves(Square from, Bitboard targets, Bitboard enemies,
                                   MoveList& moves) {
    Bitboard caps = targets & enemies;
    Bitboard quiets = targets ^ caps;
    while (caps)
        moves.push_back(Move(from, popLsb(caps), MoveType::Capture));
    while (quiets)
        moves.push_back(Move(from, popLsb(quiets), MoveType::Normal));
}

void MoveGenerator::generatePieceMoves(const Position& pos, Square from, MoveList& moves) {
    const Piece& piece = pos.pieceAt(from);
    Bitboard occupied = pos.occupied();
    Bitboard targets = pieceAttacks(piece.type(), from, occupied) & ~pos.pieces(piece.color());
    serializeMoves(from, targets, pos.pieces(~piece.color()), moves);
}

void MoveGenerator::generateCastlingMoves(const Position& pos, MoveList& moves) {
    Color us = pos.sideToMove();
    Square kingSquare = pos.kingSquare(us);

    if (kingSquare == SQUARE_NONE)
        return;

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

        if (pos.pieceAt(sq).type() == PieceType::Pawn)
            generatePawnMoves(pos, sq, moves);
        else
            generatePieceMoves(pos, sq, moves);
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
    Square kingSq = pos.kingSquare(side);
    return kingSq != SQUARE_NONE && isSquareAttacked(pos, kingSq, ~side);
}

bool MoveGenerator::moveLeavesKingInCheck(Position& pos, const Move& move) {
    Color us = pos.sideToMove();
    UndoInfo undo = pos.makeMove(move);
    bool inCheck = isInCheck(pos, us);
    pos.unmakeMove(move, undo);
    return inCheck;
}

// ============================================================================
// Capture/Promotion-Only Generation (for quiescence search)
// ============================================================================

void MoveGenerator::generatePawnCaptures(const Position& pos, Square from, MoveList& moves) {
    Color us = pos.pieceAt(from).color();
    File fromFile = getFile(from);
    Rank fromRank = getRank(from);
    int direction = (us == Color::White) ? 1 : -1;
    Rank promotionRank = (us == Color::White) ? RANK_8 : RANK_1;

    int captureRank = static_cast<int>(fromRank) + direction;
    if (captureRank < 0 || captureRank > 7)
        return;

    // Promotion pushes (non-capture promotions are tactical)
    if (static_cast<Rank>(captureRank) == promotionRank) {
        Square to = makeSquare(fromFile, static_cast<Rank>(captureRank));
        if (!testBit(pos.occupied(), to)) {
            moves.push_back(Move::makePromotion(from, to, PieceType::Queen));
            moves.push_back(Move::makePromotion(from, to, PieceType::Rook));
            moves.push_back(Move::makePromotion(from, to, PieceType::Bishop));
            moves.push_back(Move::makePromotion(from, to, PieceType::Knight));
        }
    }

    // Diagonal captures (including promotion captures and en passant)
    Bitboard enemies = pos.pieces(~us);
    for (int df : {-1, 1}) {
        int f = static_cast<int>(fromFile) + df;
        if (f < 0 || f > 7)
            continue;
        Square to = makeSquare(static_cast<File>(f), static_cast<Rank>(captureRank));

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

        if (to == pos.enPassantSquare()) {
            moves.push_back(Move::makeEnPassant(from, to));
        }
    }
}

MoveList MoveGenerator::generatePseudoLegalCaptures(const Position& pos) {
    MoveList moves;
    Color us = pos.sideToMove();
    Bitboard occupied = pos.occupied();
    Bitboard enemies = pos.pieces(~us);
    Bitboard ourPieces = pos.pieces(us);

    while (ourPieces) {
        Square sq = popLsb(ourPieces);
        if (pos.pieceAt(sq).type() == PieceType::Pawn) {
            generatePawnCaptures(pos, sq, moves);
        } else {
            Bitboard targets = pieceAttacks(pos.pieceAt(sq).type(), sq, occupied) & enemies;
            while (targets)
                moves.push_back(Move(sq, popLsb(targets), MoveType::Capture));
        }
    }

    return moves;
}

MoveList MoveGenerator::generateLegalCaptures(const Position& pos) {
    MoveList pseudoLegal = generatePseudoLegalCaptures(pos);
    MoveList legal;

    Position workspace = pos;
    std::copy_if(
        pseudoLegal.begin(), pseudoLegal.end(), std::back_inserter(legal),
        [&workspace](const Move& move) { return !moveLeavesKingInCheck(workspace, move); });

    return legal;
}

// ============================================================================
// Legal Move Generation
// ============================================================================

MoveList MoveGenerator::generateLegalMoves(const Position& pos) {
    MoveList pseudoLegal = generatePseudoLegalMoves(pos);
    MoveList legal;

    Position workspace = pos;
    std::copy_if(
        pseudoLegal.begin(), pseudoLegal.end(), std::back_inserter(legal),
        [&workspace](const Move& move) { return !moveLeavesKingInCheck(workspace, move); });

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
