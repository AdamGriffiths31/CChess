#include "mode/EngineMatch.h"

#include "ai/Eval.h"
#include "ai/Search.h"
#include "ai/SearchConfig.h"
#include "ai/TranspositionTable.h"
#include "core/Move.h"
#include "core/Notation.h"
#include "display/BoardRenderer.h"
#include "uci/UciEngine.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace cchess {

namespace {

std::string formatScore(int score) {
    if (score >= eval::SCORE_MATE - 200) {
        int matePly = eval::SCORE_MATE - score;
        int mateN = (matePly + 1) / 2;
        return "M" + std::to_string(mateN);
    }
    if (score <= -(eval::SCORE_MATE - 200)) {
        int matePly = eval::SCORE_MATE + score;
        int mateN = (matePly + 1) / 2;
        return "-M" + std::to_string(mateN);
    }
    std::ostringstream oss;
    oss << std::showpos << std::fixed << std::setprecision(2)
        << (static_cast<double>(score) / 100.0);
    return oss.str();
}

std::string compactNumber(uint64_t n) {
    if (n >= 1'000'000) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (static_cast<double>(n) / 1'000'000.0) << "M";
        return oss.str();
    }
    if (n >= 10'000) {
        return std::to_string(n / 1000) + "k";
    }
    if (n >= 1000) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << (static_cast<double>(n) / 1000.0) << "k";
        return oss.str();
    }
    return std::to_string(n);
}

std::string commaNumber(uint64_t n) {
    std::string s = std::to_string(n);
    int insertPos = static_cast<int>(s.length()) - 3;
    while (insertPos > 0) {
        s.insert(static_cast<size_t>(insertPos), ",");
        insertPos -= 3;
    }
    return s;
}

std::string timestamp(const char* fmt) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, fmt);
    return oss.str();
}

}  // namespace

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
    moveLog_.clear();

    int wtime = timeMs_;
    int btime = timeMs_;
    std::string result;

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
            result = winner == Color::White ? "White (CChess) wins by checkmate"
                                            : "Black (" + opponent_.name + ") wins by checkmate";
            break;
        }
        if (board_.isStalemate()) {
            std::cout << "\n*** STALEMATE! ***\n";
            std::cout << "Game drawn.\n";
            result = "Draw by stalemate";
            break;
        }
        if (board_.isDraw()) {
            std::cout << "\n*** DRAW! ***\n";
            std::cout << "50-move rule: Game drawn.\n";
            result = "Draw by 50-move rule";
            break;
        }

        if (board_.isInCheck()) {
            std::cout << ">>> CHECK! <<<\n";
        }

        std::string moveUci;
        auto moveStart = std::chrono::steady_clock::now();

        int moveNumber = board_.fullmoveNumber();

        if (board_.sideToMove() == Color::White) {
            // CChess's turn — use internal search with time management
            SearchConfig config;
            config.searchTime = std::chrono::milliseconds(allocateTime(wtime, incMs_));

            // Capture search info via callback
            SearchInfo lastInfo{};
            auto infoCallback = [&lastInfo](const SearchInfo& info) { lastInfo = info; };

            Search search(board_, config, tt, infoCallback);
            Move best = search.findBestMove();

            std::string san = moveToSan(board_, best);
            moveUci = best.toAlgebraic();
            board_.makeMoveUnchecked(best);

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - moveStart)
                               .count();
            wtime -= static_cast<int>(elapsed);
            wtime += incMs_;

            int timeMs = std::max(static_cast<int>(elapsed), 1);
            uint64_t nps = lastInfo.nodes * 1000 / static_cast<uint64_t>(timeMs);

            // Record move
            MoveRecord rec;
            rec.moveNumber = moveNumber;
            rec.san = san;
            rec.uci = moveUci;
            rec.side = Color::White;
            rec.depthReached = lastInfo.depth;
            rec.score = lastInfo.score;
            rec.nodes = lastInfo.nodes;
            rec.timeMs = static_cast<int>(elapsed);
            rec.nps = nps;
            rec.pv = lastInfo.pv;
            rec.hasCChessInfo = true;
            moveLog_.push_back(std::move(rec));

            // Print per-move metrics
            std::cout << "\nCChess plays: " << san << " (" << moveUci << ")"
                      << " | depth " << lastInfo.depth << " | score " << formatScore(lastInfo.score)
                      << " | " << lastInfo.nodes << " nodes | " << elapsed << "ms"
                      << " | " << compactNumber(nps) << "NPS\n";

            if (wtime <= 0) {
                std::cout << "\n*** White lost on time! ***\n";
                std::cout << opponent_.name << " wins!\n";
                result = "Black (" + opponent_.name + ") wins on time";
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
                result = "Game aborted (no move from engine)";
                break;
            }

            // Get SAN before applying move
            std::string san;
            auto parsed = Move::fromAlgebraic(moveUci);
            if (parsed) {
                PieceType promo = parsed->isPromotion() ? parsed->promotion() : PieceType::None;
                auto legal = board_.findLegalMove(parsed->from(), parsed->to(), promo);
                if (legal) {
                    san = moveToSan(board_, *legal);
                    board_.makeMoveUnchecked(*legal);
                } else {
                    std::cout << "\nEngine returned illegal move: " << moveUci
                              << ". Ending game.\n";
                    result = "Game aborted (illegal move from engine)";
                    break;
                }
            } else {
                std::cout << "\nEngine returned unparseable move: " << moveUci
                          << ". Ending game.\n";
                result = "Game aborted (unparseable move from engine)";
                break;
            }

            // Record opponent move
            MoveRecord rec;
            rec.moveNumber = moveNumber;
            rec.san = san;
            rec.uci = moveUci;
            rec.side = Color::Black;
            rec.timeMs = static_cast<int>(elapsed);
            rec.hasCChessInfo = false;
            moveLog_.push_back(std::move(rec));

            std::cout << "\n"
                      << opponent_.name << " plays: " << san << " (" << moveUci << ")"
                      << " [" << elapsed << "ms]\n";

            if (btime <= 0) {
                std::cout << "\n*** Black lost on time! ***\n";
                std::cout << "CChess wins!\n";
                result = "White (CChess) wins on time";
                break;
            }
        }

        moveHistory.push_back(moveUci);
    }

    std::cout << "\n=== Game Over ===\n";

    // Print game summary to console
    auto summary = GameSummary::fromLog(moveLog_);

    std::cout << "\n--- Game Summary ---\n";
    std::cout << "Total moves: " << moveLog_.size() << "\n";
    if (summary.cchessMoves > 0) {
        std::cout << "CChess nodes (total): " << commaNumber(summary.totalNodes) << "\n";
        std::cout << "CChess avg depth: " << std::fixed << std::setprecision(1)
                  << (static_cast<double>(summary.totalDepth) / summary.cchessMoves) << "\n";
        std::cout << "CChess avg NPS: "
                  << commaNumber(summary.totalNps / static_cast<uint64_t>(summary.cchessMoves))
                  << "\n";
        std::cout << "CChess avg time/move: " << (summary.totalTimeMs / summary.cchessMoves)
                  << "ms\n";
    }
    auto ttStats = tt.stats();
    std::cout << "TT hit rate: " << std::fixed << std::setprecision(1) << ttStats.hitRate()
              << "%\n";
    std::cout << "TT cutoff rate: " << std::fixed << std::setprecision(1) << ttStats.cutoffRate()
              << "%\n";
    std::cout << "TT occupancy: " << std::fixed << std::setprecision(1) << tt.occupancy() << "%\n";

    // Write markdown report
    writeGameReport(result, tt.stats(), tt.occupancy());
}

void EngineMatch::writeGameReport(const std::string& result, const TTStats& ttStats,
                                  double ttOccupancy) const {
    std::filesystem::create_directories("results");
    std::string filename = "results/game_" + timestamp("%Y%m%d_%H%M%S") + ".md";
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cout << "Warning: could not write game report to " << filename << "\n";
        return;
    }

    int baseSec = timeMs_ / 1000;
    int incSec = incMs_ / 1000;

    out << "# CChess vs " << opponent_.name << "\n";
    out << "**Date:** " << timestamp("%Y-%m-%d %H:%M:%S") << "\n";
    out << "**Time Control:** " << baseSec << "+" << incSec << "\n";
    out << "**Result:** " << result << "\n\n";

    // Move log table
    out << "## Move Log\n\n";
    out << "| # | Move | Depth | Score | Nodes | Time | NPS | PV |\n";
    out << "|---|------|-------|-------|-------|------|-----|----|\n";

    for (const auto& rec : moveLog_) {
        std::string moveLabel =
            std::to_string(rec.moveNumber) + (rec.side == Color::White ? "." : "...");

        if (rec.hasCChessInfo) {
            // Build PV string (up to 5 moves)
            std::string pvStr;
            int pvCount = std::min(static_cast<int>(rec.pv.size()), 5);
            for (int i = 0; i < pvCount; ++i) {
                if (i > 0)
                    pvStr += " ";
                pvStr += rec.pv[static_cast<size_t>(i)].toAlgebraic();
            }

            out << "| " << moveLabel << " | " << rec.san << " | " << rec.depthReached << " | "
                << formatScore(rec.score) << " | " << compactNumber(rec.nodes) << " | "
                << rec.timeMs << "ms | " << compactNumber(rec.nps) << " | " << pvStr << " |\n";
        } else {
            out << "| " << moveLabel << " | " << rec.san << " | — | — | — | " << rec.timeMs
                << "ms | — | — |\n";
        }
    }

    // Summary table
    auto summary = GameSummary::fromLog(moveLog_);

    out << "\n## Summary\n\n";
    out << "| Metric | Value |\n";
    out << "|--------|-------|\n";
    out << "| Total Moves | " << moveLog_.size() << " |\n";
    if (summary.cchessMoves > 0) {
        out << "| CChess Nodes (total) | " << commaNumber(summary.totalNodes) << " |\n";
        out << "| CChess Avg Depth | " << std::fixed << std::setprecision(1)
            << (static_cast<double>(summary.totalDepth) / summary.cchessMoves) << " |\n";
        out << "| CChess Avg NPS | "
            << commaNumber(summary.totalNps / static_cast<uint64_t>(summary.cchessMoves)) << " |\n";
        out << "| CChess Avg Time/Move | " << (summary.totalTimeMs / summary.cchessMoves)
            << "ms |\n";
    }

    out << std::fixed << std::setprecision(1);
    out << "| TT Hit Rate | " << ttStats.hitRate() << "% |\n";
    out << "| TT Cutoff Rate | " << ttStats.cutoffRate() << "% |\n";
    out << "| TT Occupancy | " << ttOccupancy << "% |\n";

    std::cout << "Game report saved to: " << filename << "\n";
}

}  // namespace cchess
