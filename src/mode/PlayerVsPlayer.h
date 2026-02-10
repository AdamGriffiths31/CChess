#ifndef CCHESS_PLAYERVSPLAYER_H
#define CCHESS_PLAYERVSPLAYER_H

#include "../core/Board.h"
#include "../core/Move.h"

#include <string>

namespace cchess {

class PlayerVsPlayer {
public:
    // Constructor
    explicit PlayerVsPlayer(const std::string& fen = Board::STARTING_FEN);

    // Run the game loop
    void play();

private:
    // Game state
    Board board_;

    // Display the current position
    void displayBoard() const;

    // Get move input from current player
    bool getMoveInput(std::string& from, std::string& to);

    // Execute a move
    bool makeMove(const std::string& from, const std::string& to);

    // Create a Move object from user input
    Move createMoveFromInput(Square from, Square to);

    // Handle promotion input
    static PieceType getPromotionChoice();
};

}  // namespace cchess

#endif  // CCHESS_PLAYERVSPLAYER_H
