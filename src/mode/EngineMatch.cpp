#include "mode/EngineMatch.h"

#include "ai/Search.h"
#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Move.h"
#include "core/Notation.h"
#include "display/BoardRenderer.h"
#include "uci/UciEngine.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace cchess {

EngineMatch::EngineMatch(const Opponent& opponent, int timeMs, int incMs)
    : opponent_(opponent), timeMs_(timeMs), incMs_(incMs) {}

int EngineMatch::allocateTime(int remainingMs, int incMs) const {
    int allocated = remainingMs / 30 + incMs;
    allocated = std::min(allocated, remainingMs / 3);
    return std::max(allocated, 50);  // at least 50ms
}

void EngineMatch::play() {
    int baseSec = timeMs_ / 1000;
    int incSec = incMs_ / 1000;
    std::cout << "\n=== CChess vs " << opponent_.name << " ===\n";
    std::cout << "CChess plays White | " << opponent_.name << " plays Black\n";
    std::cout << "Time control: " << baseSec << "+" << incSec << "\n\n";

    // Start opponent engine
    UciEngine engine(opponent_.enginePath);
    engine.initUci();
    for (const auto& [name, value] : opponent_.options) {
        engine.setOption(name, value);
    }
    engine.newGame();

    TranspositionTable tt;
    std::vector<std::string> moveHistory;

    int wtime = timeMs_;
    int btime = timeMs_;

    while (true) {
        // Display board
        std::cout << "\n" << BoardRenderer::render(board_) << "\n";
        std::cout << BoardRenderer::renderPositionInfo(board_);
        std::cout << "Clock: White " << wtime / 1000 << "." << (wtime / 100) % 10 << "s"
                  << "  Black " << btime / 1000 << "." << (btime / 100) % 10 << "s\n";

        // Check game termination
        if (board_.isCheckmate()) {
            Color winner = ~board_.sideToMove();
            std::cout << "\n*** CHECKMATE! ***\n";
            std::cout << (winner == Color::White ? "White (CChess)"
                                                 : "Black (" + opponent_.name + ")")
                      << " wins!\n";
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

        if (board_.isInCheck()) {
            std::cout << ">>> CHECK! <<<\n";
        }

        std::string moveUci;
        auto moveStart = std::chrono::steady_clock::now();

        if (board_.sideToMove() == Color::White) {
            // CChess's turn — use internal search with time management
            SearchConfig config;
            config.searchTime = std::chrono::milliseconds(allocateTime(wtime, incMs_));
            Search search(board_, config, tt);
            Move best = search.findBestMove();

            std::string san = moveToSan(board_, best);
            moveUci = best.toAlgebraic();
            board_.makeMoveUnchecked(best);

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - moveStart)
                               .count();
            wtime -= static_cast<int>(elapsed);
            wtime += incMs_;

            std::cout << "\nCChess plays: " << san << " (" << moveUci << ")"
                      << " [" << elapsed << "ms]\n";

            if (wtime <= 0) {
                std::cout << "\n*** White lost on time! ***\n";
                std::cout << opponent_.name << " wins!\n";
                break;
            }
        } else {
            // Opponent engine's turn
            std::string posCmd = "startpos";
            if (!moveHistory.empty()) {
                posCmd += " moves";
                for (const auto& m : moveHistory) {
                    posCmd += " " + m;
                }
            }
            engine.send("position " + posCmd);
            engine.send("isready");
            engine.readUntil("readyok");

            moveUci =
                engine.go("wtime " + std::to_string(wtime) + " btime " + std::to_string(btime) +
                          " winc " + std::to_string(incMs_) + " binc " + std::to_string(incMs_));

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - moveStart)
                               .count();
            btime -= static_cast<int>(elapsed);
            btime += incMs_;

            if (moveUci.empty()) {
                std::cout << "\nEngine returned no move. Ending game.\n";
                break;
            }

            if (!applyUciMove(moveUci)) {
                std::cout << "\nEngine returned illegal move: " << moveUci << ". Ending game.\n";
                break;
            }

            std::cout << "\n"
                      << opponent_.name << " plays: " << moveUci << " [" << elapsed << "ms]\n";

            if (btime <= 0) {
                std::cout << "\n*** Black lost on time! ***\n";
                std::cout << "CChess wins!\n";
                break;
            }
        }

        moveHistory.push_back(moveUci);
    }

    std::cout << "\n=== Game Over ===\n";
}

bool EngineMatch::applyUciMove(const std::string& uciMove) {
    auto parsed = Move::fromAlgebraic(uciMove);
    if (!parsed)
        return false;

    PieceType promo = parsed->isPromotion() ? parsed->promotion() : PieceType::None;
    auto legal = board_.findLegalMove(parsed->from(), parsed->to(), promo);
    if (!legal)
        return false;

    board_.makeMoveUnchecked(*legal);
    return true;
}

}  // namespace cchess
