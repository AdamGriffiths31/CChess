#include "PerftRunner.h"

#include "../core/Square.h"

#include <chrono>
#include <iostream>
#include <limits>

namespace cchess {

void PerftRunner::run() {
    std::cout << "=== Perft Test ===\n";
    std::cout << "Enter FEN (or press Enter for starting position): ";

    std::string fen;
    std::getline(std::cin, fen);
    if (fen.empty()) {
        fen = Board::STARTING_FEN;
    }

    Board board(fen);
    std::cout << "Position: " << fen << "\n";

    std::cout << "Enter depth (1-7): ";
    int depth;
    while (!(std::cin >> depth) || depth < 1 || depth > 7) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid depth. Enter 1-7: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "\nRunning perft(" << depth << ")...\n\n";

    divide(board, depth);
}

uint64_t PerftRunner::perft(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    auto moves = board.getLegalMoves();

    if (depth == 1) {
        return moves.size();
    }

    uint64_t nodes = 0;
    for (const Move& move : moves) {
        UndoInfo undo = board.makeMoveUnchecked(move);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    return nodes;
}

void PerftRunner::divide(Board& board, int depth) {
    auto start = std::chrono::steady_clock::now();

    auto moves = board.getLegalMoves();
    uint64_t total = 0;

    for (const Move& move : moves) {
        UndoInfo undo = board.makeMoveUnchecked(move);

        uint64_t nodes = (depth <= 1) ? 1 : perft(board, depth - 1);
        total += nodes;

        board.unmakeMove(move, undo);

        std::cout << squareToString(move.from()) << squareToString(move.to()) << ": " << nodes
                  << "\n";
    }

    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "\nNodes: " << total << "\n";
    std::cout << "Time:  " << ms << " ms\n";
    if (ms > 0) {
        std::cout << "NPS:   " << (total * 1000 / static_cast<uint64_t>(ms)) << "\n";
    }
}

}  // namespace cchess
