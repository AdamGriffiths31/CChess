#include "PlayerVsPlayer.h"

#include "../core/Square.h"
#include "../display/BoardRenderer.h"

#include <iostream>
#include <sstream>

namespace cchess {

PlayerVsPlayer::PlayerVsPlayer(const std::string& fen) : board_(fen) {}

void PlayerVsPlayer::play() {
    std::cout << "=== Chess: Player vs Player ===\n";
    std::cout << "Enter moves as: <from> <to> (e.g., e2 e4)\n";
    std::cout << "Enter 'quit' to exit\n\n";

    while (true) {
        displayBoard();

        // Check game termination conditions
        if (board_.isCheckmate()) {
            Color winner = ~board_.sideToMove();  // The side that just moved won
            std::cout << "\n*** CHECKMATE! ***\n";
            std::cout << (winner == Color::White ? "White" : "Black") << " wins!\n";
            break;
        }

        if (board_.isStalemate()) {
            std::cout << "\n*** STALEMATE! ***\n";
            std::cout << "Game drawn.\n";
            break;
        }

        if (board_.isDraw()) {
            std::cout << "\n*** DRAW! ***\n";
            std::cout << "50-move rule: Game drawn.\n";
            break;
        }

        // Warn about check
        if (board_.isInCheck()) {
            std::cout << "\n>>> CHECK! <<<\n";
        }

        // Get and make move
        std::string from, to;
        if (!getMoveInput(from, to)) {
            std::cout << "Game ended.\n";
            break;
        }

        if (!makeMove(from, to)) {
            std::cout << "Invalid move. Try again.\n";
            continue;
        }
    }
}

void PlayerVsPlayer::displayBoard() const {
    std::cout << "\n" << BoardRenderer::render(board_) << "\n";
    std::cout << BoardRenderer::renderPositionInfo(board_);
}

bool PlayerVsPlayer::getMoveInput(std::string& from, std::string& to) {
    Color currentPlayer = board_.sideToMove();

    while (true) {
        std::cout << "\n" << (currentPlayer == Color::White ? "White" : "Black") << "'s turn: ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            return false;
        }

        if (input == "quit" || input == "exit") {
            return false;
        }

        std::istringstream iss(input);
        if (!(iss >> from >> to)) {
            std::cout << "Invalid input format. Use: <from> <to> (e.g., e2 e4)\n";
            continue;  // Loop instead of recursion
        }

        return true;
    }
}

bool PlayerVsPlayer::makeMove(const std::string& from, const std::string& to) {
    try {
        // Get squares
        auto fromSq = stringToSquare(from);
        auto toSq = stringToSquare(to);

        if (!fromSq || !toSq) {
            std::cout << "Invalid square notation.\n";
            return false;
        }

        // Get piece at source square
        const Piece& piece = board_.at(*fromSq);
        if (piece.isEmpty()) {
            std::cout << "No piece at " << from << "\n";
            return false;
        }

        // Check if piece belongs to current player
        if (piece.color() != board_.sideToMove()) {
            std::cout << "That's not your piece!\n";
            return false;
        }

        // Create Move object
        Move move = createMoveFromInput(*fromSq, *toSq);

        // Validate and execute move via Board
        if (!board_.makeMove(move)) {
            std::cout << "Illegal move";
            if (board_.isInCheck()) {
                std::cout << " (your king is in check)";
            }
            std::cout << ".\n";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return false;
    }
}

Move PlayerVsPlayer::createMoveFromInput(Square from, Square to) {
    const Piece& piece = board_.at(from);
    const Piece& target = board_.at(to);

    // Detect castling (king moves 2 squares horizontally)
    if (piece.type() == PieceType::King) {
        int fileDiff = std::abs(static_cast<int>(getFile(to)) - static_cast<int>(getFile(from)));
        if (fileDiff == 2) {
            return Move::makeCastling(from, to);
        }
    }

    // Detect en passant (pawn diagonal move to en passant square)
    if (piece.type() == PieceType::Pawn && to == board_.enPassantSquare()) {
        return Move::makeEnPassant(from, to);
    }

    // Detect promotion (pawn reaches last rank)
    if (piece.type() == PieceType::Pawn) {
        Rank toRank = getRank(to);
        Color us = board_.sideToMove();
        Rank promotionRank = (us == Color::White) ? RANK_8 : RANK_1;
        if (toRank == promotionRank) {
            PieceType promotion = getPromotionChoice();
            if (target.isEmpty()) {
                return Move::makePromotion(from, to, promotion);
            } else {
                return Move::makePromotionCapture(from, to, promotion);
            }
        }
    }

    // Detect capture
    if (!target.isEmpty()) {
        return Move(from, to, MoveType::Capture);
    }

    // Normal move
    return Move(from, to, MoveType::Normal);
}

PieceType PlayerVsPlayer::getPromotionChoice() {
    std::cout << "Promote to (Q/R/B/N): ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return PieceType::Queen;  // Default to queen
    }

    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(choice[0])));
    switch (c) {
        case 'q':
            return PieceType::Queen;
        case 'r':
            return PieceType::Rook;
        case 'b':
            return PieceType::Bishop;
        case 'n':
            return PieceType::Knight;
        default:
            std::cout << "Invalid choice. Promoting to Queen.\n";
            return PieceType::Queen;
    }
}

}  // namespace cchess
