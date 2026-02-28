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
    if (n >= 10'000)
        return std::to_string(n / 1000) + "k";
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
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, fmt);
    return oss.str();
}

}  // namespace

// ---------------------------------------------------------------------------

EngineMatch::EngineMatch(const Opponent& opponent, int timeMs, int incMs)
    : opponent_(opponent), timeMs_(timeMs), incMs_(incMs) {}

int EngineMatch::allocateTime(int remainingMs, int incMs) const {
    int allocated = remainingMs / 30 + incMs;
    allocated = std::min(allocated, remainingMs / 3);
    return std::max(allocated, 50);
}

// ---------------------------------------------------------------------------
// Single game
// ---------------------------------------------------------------------------

GameResult EngineMatch::playGame(Color cchessColor, int gameNumber) {
    Board board;
    std::vector<MoveRecord> log;

    const std::string cchessSide = (cchessColor == Color::White) ? "White" : "Black";
    const std::string oppSide = (cchessColor == Color::White) ? "Black" : "White";

    std::cout << "\n=== Game " << gameNumber << ": CChess(" << cchessSide << ") vs "
              << opponent_.name << "(" << oppSide << ") ===\n";
    std::cout << "Time control: " << timeMs_ / 1000 << "+" << incMs_ / 1000 << "\n";
    if (!opponent_.options.empty()) {
        std::cout << "Opponent options:";
        for (const auto& [name, value] : opponent_.options)
            std::cout << "  " << name << "=" << value;
        std::cout << "\n";
    }
    std::cout << "\n";

    UciEngine engine(opponent_.enginePath);
    engine.initUci();
    for (const auto& [name, value] : opponent_.options)
        engine.setOption(name, value);
    engine.newGame();

    TranspositionTable tt;
    std::vector<std::string> moveHistory;

    int wtime = timeMs_;
    int btime = timeMs_;
    std::string resultStr;
    int score = 0;
    bool aborted = false;

    while (true) {
        std::cout << "\n" << BoardRenderer::render(board) << "\n";
        std::cout << BoardRenderer::renderPositionInfo(board);
        std::cout << "Clock: White " << wtime / 1000 << "." << (wtime / 100) % 10 << "s"
                  << "  Black " << btime / 1000 << "." << (btime / 100) % 10 << "s\n";

        if (board.isCheckmate()) {
            Color winner = ~board.sideToMove();
            bool cchessWon = (winner == cchessColor);
            std::cout << "\n*** CHECKMATE! " << (cchessWon ? "CChess" : opponent_.name)
                      << " wins! ***\n";
            resultStr = cchessWon ? "CChess (" + cchessSide + ") wins by checkmate"
                                  : opponent_.name + " (" + oppSide + ") wins by checkmate";
            score = cchessWon ? 1 : -1;
            break;
        }
        if (board.isStalemate()) {
            std::cout << "\n*** STALEMATE — draw ***\n";
            resultStr = "Draw by stalemate";
            score = 0;
            break;
        }
        if (board.isDraw()) {
            std::cout << "\n*** DRAW (50-move rule) ***\n";
            resultStr = "Draw by 50-move rule";
            score = 0;
            break;
        }
        if (board.isInCheck())
            std::cout << ">>> CHECK! <<<\n";

        auto moveStart = std::chrono::steady_clock::now();
        int moveNumber = board.fullmoveNumber();
        bool isCChessTurn = (board.sideToMove() == cchessColor);
        std::string moveUci;

        if (isCChessTurn) {
            int remaining = (cchessColor == Color::White) ? wtime : btime;
            SearchConfig config;
            config.searchTime = std::chrono::milliseconds(allocateTime(remaining, incMs_));

            SearchInfo lastInfo{};
            Search search(board, config, tt, [&lastInfo](const SearchInfo& i) { lastInfo = i; });
            Move best = search.findBestMove();
            uint64_t totalNodes = search.totalNodes();

            std::string san = moveToSan(board, best);
            moveUci = best.toAlgebraic();
            board.makeMoveUnchecked(best);

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - moveStart)
                               .count();

            int& ourTime = (cchessColor == Color::White) ? wtime : btime;
            ourTime -= static_cast<int>(elapsed);
            ourTime += incMs_;

            int timeMs = std::max(static_cast<int>(elapsed), 1);
            uint64_t nps = totalNodes * 1000 / static_cast<uint64_t>(timeMs);

            MoveRecord rec;
            rec.moveNumber = moveNumber;
            rec.san = san;
            rec.uci = moveUci;
            rec.side = cchessColor;
            rec.depthReached = lastInfo.depth;
            rec.score = lastInfo.score;
            rec.nodes = totalNodes;
            rec.timeMs = static_cast<int>(elapsed);
            rec.nps = nps;
            rec.pv = lastInfo.pv;
            rec.hasCChessInfo = true;
            log.push_back(std::move(rec));

            std::cout << "\nCChess: " << san << " (" << moveUci << ")"
                      << " | depth " << lastInfo.depth << " | score " << formatScore(lastInfo.score)
                      << " | " << totalNodes << " nodes"
                      << " | " << elapsed << "ms"
                      << " | " << compactNumber(nps) << " NPS\n";

            if (ourTime <= 0) {
                std::cout << "\n*** CChess lost on time! ***\n";
                resultStr = opponent_.name + " (" + oppSide + ") wins on time";
                score = -1;
                break;
            }
        } else {
            // Opponent's turn
            std::string posCmd = "startpos";
            if (!moveHistory.empty()) {
                posCmd += " moves";
                for (const auto& m : moveHistory)
                    posCmd += " " + m;
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

            Color oppColor = ~cchessColor;
            int& oppTime = (oppColor == Color::White) ? wtime : btime;
            oppTime -= static_cast<int>(elapsed);
            oppTime += incMs_;

            if (moveUci.empty()) {
                std::cout << "\nEngine returned no move. Ending game.\n";
                resultStr = "Game aborted (no move from engine)";
                aborted = true;
                break;
            }

            std::string san;
            auto parsed = Move::fromAlgebraic(moveUci);
            if (parsed) {
                PieceType promo = parsed->isPromotion() ? parsed->promotion() : PieceType::None;
                auto legal = board.findLegalMove(parsed->from(), parsed->to(), promo);
                if (legal) {
                    san = moveToSan(board, *legal);
                    board.makeMoveUnchecked(*legal);
                } else {
                    std::cout << "\nIllegal move from engine: " << moveUci << ". Ending game.\n";
                    resultStr = "Game aborted (illegal move from engine)";
                    aborted = true;
                    break;
                }
            } else {
                std::cout << "\nUnparseable move from engine: " << moveUci << ". Ending game.\n";
                resultStr = "Game aborted (unparseable move from engine)";
                aborted = true;
                break;
            }

            MoveRecord rec;
            rec.moveNumber = moveNumber;
            rec.san = san;
            rec.uci = moveUci;
            rec.side = ~cchessColor;
            rec.timeMs = static_cast<int>(elapsed);
            rec.hasCChessInfo = false;
            log.push_back(std::move(rec));

            std::cout << "\n"
                      << opponent_.name << ": " << san << " (" << moveUci << ") [" << elapsed
                      << "ms]\n";

            if (oppTime <= 0) {
                std::cout << "\n*** " << opponent_.name << " lost on time! CChess wins! ***\n";
                resultStr = "CChess (" + cchessSide + ") wins on time";
                score = 1;
                break;
            }
        }

        moveHistory.push_back(moveUci);
    }

    auto summary = GameSummary::fromLog(log);
    auto ttStats = tt.stats();

    std::cout << "\n--- Game " << gameNumber << " summary ---\n";
    std::cout << "Result: " << resultStr << "\n";
    std::cout << "Total moves: " << log.size() << "\n";
    if (summary.cchessMoves > 0) {
        std::cout << "CChess nodes: " << commaNumber(summary.totalNodes) << "\n";
        std::cout << "CChess avg depth: " << std::fixed << std::setprecision(1)
                  << (static_cast<double>(summary.totalDepth) / summary.cchessMoves) << "\n";
        std::cout << "CChess avg NPS: "
                  << commaNumber(summary.totalNps / static_cast<uint64_t>(summary.cchessMoves))
                  << "\n";
        std::cout << "CChess avg time/move: " << (summary.totalTimeMs / summary.cchessMoves)
                  << "ms\n";
    }
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "TT hit rate: " << ttStats.hitRate() << "%  "
              << "cutoff: " << ttStats.cutoffRate() << "%  "
              << "occupancy: " << tt.occupancy() << "%\n";

    GameResult result;
    result.resultStr = resultStr;
    result.score = score;
    result.aborted = aborted;
    result.cchessColor = cchessColor;
    result.gameNumber = gameNumber;
    result.summary = summary;
    result.ttStats = ttStats;
    result.ttOccupancy = tt.occupancy();

    writeGameReport(result, log);
    return result;
}

// ---------------------------------------------------------------------------
// Series
// ---------------------------------------------------------------------------

void EngineMatch::playSeries() {
    const std::vector<Color> sides = {Color::White, Color::Black, Color::White};

    std::cout << "\n=== Engine Match: CChess vs " << opponent_.name << " ===\n";
    std::cout << "3 games  |  " << timeMs_ / 1000 << "+" << incMs_ / 1000 << "  |  CChess: W B W\n";

    int wins = 0, draws = 0, losses = 0;
    std::vector<GameResult> results;

    for (int g = 0; g < 3; ++g) {
        GameResult r = playGame(sides[static_cast<size_t>(g)], g + 1);
        results.push_back(r);
        if (!r.aborted) {
            if (r.score == 1)
                ++wins;
            else if (r.score == 0)
                ++draws;
            else
                ++losses;
        }
        std::cout << "\nSeries: CChess " << wins << "W / " << draws << "D / " << losses << "L"
                  << (r.aborted ? " (game aborted)" : "") << "\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "=== Series result: CChess vs " << opponent_.name << " ===\n";
    std::cout << "CChess " << wins << "W / " << draws << "D / " << losses << "L  =>  ";
    if (wins > losses)
        std::cout << "CChess wins the series!\n";
    else if (losses > wins)
        std::cout << opponent_.name << " wins the series!\n";
    else
        std::cout << "Series drawn.\n";

    std::cout << "\nPer-game:\n";
    for (const auto& r : results) {
        const char* side = (r.cchessColor == Color::White) ? "White" : "Black";
        const char* outcome = r.aborted        ? "Aborted"
                              : (r.score == 1) ? "Win"
                              : (r.score == 0) ? "Draw"
                                               : "Loss";
        std::cout << "  Game " << r.gameNumber << " (CChess=" << side << "): " << outcome << " — "
                  << r.resultStr << "\n";
        if (r.summary.cchessMoves > 0) {
            std::cout << "    avg NPS: "
                      << commaNumber(r.summary.totalNps /
                                     static_cast<uint64_t>(r.summary.cchessMoves))
                      << "  TT hit: " << std::fixed << std::setprecision(1) << r.ttStats.hitRate()
                      << "%"
                      << "  TT occ: " << r.ttOccupancy << "%\n";
        }
    }

    appendSeriesRecord(results);
}

// ---------------------------------------------------------------------------
// Reports
// ---------------------------------------------------------------------------

void EngineMatch::writeGameReport(const GameResult& result,
                                  const std::vector<MoveRecord>& log) const {
    std::filesystem::create_directories("results");
    std::string filename = "results/game_" + timestamp("%Y%m%d_%H%M%S") + ".md";
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cout << "Warning: could not write game report to " << filename << "\n";
        return;
    }

    const std::string cchessSide = (result.cchessColor == Color::White) ? "White" : "Black";
    const std::string oppSide = (result.cchessColor == Color::White) ? "Black" : "White";

    out << "# Game " << result.gameNumber << ": CChess vs " << opponent_.name << "\n";
    out << "**Date:** " << timestamp("%Y-%m-%d %H:%M:%S") << "\n";
    out << "**Time Control:** " << timeMs_ / 1000 << "+" << incMs_ / 1000 << "\n";
    out << "**CChess plays:** " << cchessSide << "\n";
    out << "**Opponent plays:** " << oppSide << "\n";
    out << "**Result:** " << result.resultStr << "\n";

    if (!opponent_.options.empty()) {
        out << "\n### Opponent UCI Options\n\n";
        out << "| Option | Value |\n|--------|-------|\n";
        for (const auto& [name, value] : opponent_.options)
            out << "| " << name << " | " << value << " |\n";
    }

    out << "\n## Move Log\n\n";
    out << "| # | Move | Depth | Score | Nodes | Time | NPS | PV |\n";
    out << "|---|------|-------|-------|-------|------|-----|----|\n";

    for (const auto& rec : log) {
        std::string label =
            std::to_string(rec.moveNumber) + (rec.side == Color::White ? "." : "...");
        if (rec.hasCChessInfo) {
            std::string pvStr;
            int pvCount = std::min(static_cast<int>(rec.pv.size()), 5);
            for (int i = 0; i < pvCount; ++i) {
                if (i > 0)
                    pvStr += " ";
                pvStr += rec.pv[static_cast<size_t>(i)].toAlgebraic();
            }
            out << "| " << label << " | " << rec.san << " | " << rec.depthReached << " | "
                << formatScore(rec.score) << " | " << compactNumber(rec.nodes) << " | "
                << rec.timeMs << "ms"
                << " | " << compactNumber(rec.nps) << " | " << pvStr << " |\n";
        } else {
            out << "| " << label << " | " << rec.san << " | — | — | — | " << rec.timeMs
                << "ms | — | — |\n";
        }
    }

    const auto& s = result.summary;
    out << "\n## Summary\n\n";
    out << "| Metric | Value |\n|--------|-------|\n";
    out << "| Total Moves | " << log.size() << " |\n";
    if (s.cchessMoves > 0) {
        out << "| CChess Nodes (total) | " << commaNumber(s.totalNodes) << " |\n";
        out << "| CChess Avg Depth | " << std::fixed << std::setprecision(1)
            << (static_cast<double>(s.totalDepth) / s.cchessMoves) << " |\n";
        out << "| CChess Avg NPS | "
            << commaNumber(s.totalNps / static_cast<uint64_t>(s.cchessMoves)) << " |\n";
        out << "| CChess Avg Time/Move | " << (s.totalTimeMs / s.cchessMoves) << "ms |\n";
    }
    out << std::fixed << std::setprecision(1);
    out << "| TT Hit Rate | " << result.ttStats.hitRate() << "% |\n";
    out << "| TT Cutoff Rate | " << result.ttStats.cutoffRate() << "% |\n";
    out << "| TT Occupancy | " << result.ttOccupancy << "% |\n";

    std::cout << "Game report saved to: " << filename << "\n";

    writePgn(result, log);
}

void EngineMatch::writePgn(const GameResult& result, const std::vector<MoveRecord>& log) const {
    std::filesystem::create_directories("results");
    std::string filename = "results/game_" + timestamp("%Y%m%d_%H%M%S") + ".pgn";
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cout << "Warning: could not write PGN to " << filename << "\n";
        return;
    }

    // Determine White/Black player names
    const bool cchessIsWhite = (result.cchessColor == Color::White);
    const std::string whiteName = cchessIsWhite ? "CChess" : opponent_.name;
    const std::string blackName = cchessIsWhite ? opponent_.name : "CChess";

    // PGN result token — aborted games stay as "*"
    std::string pgnResult = "*";
    if (!result.aborted) {
        if (result.score == 1)
            pgnResult = cchessIsWhite ? "1-0" : "0-1";
        else if (result.score == -1)
            pgnResult = cchessIsWhite ? "0-1" : "1-0";
        else
            pgnResult = "1/2-1/2";
    }

    // Seven tag roster
    out << "[Event \"CChess Engine Match\"]\n";
    out << "[Site \"Local\"]\n";
    out << "[Date \"" << timestamp("%Y.%m.%d") << "\"]\n";
    out << "[Round \"" << result.gameNumber << "\"]\n";
    out << "[White \"" << whiteName << "\"]\n";
    out << "[Black \"" << blackName << "\"]\n";
    out << "[Result \"" << pgnResult << "\"]\n";
    out << "[TimeControl \"" << timeMs_ / 1000 << "+" << incMs_ / 1000 << "\"]\n";
    // Opponent UCI options as a comment tag
    if (!opponent_.options.empty()) {
        std::string optStr;
        for (const auto& [k, v] : opponent_.options) {
            if (!optStr.empty())
                optStr += ", ";
            optStr += k + "=" + v;
        }
        out << "[" << opponent_.name << "Options \"" << optStr << "\"]\n";
    }
    out << "\n";

    // Move text — wrap at ~80 chars per line
    std::string line;
    auto flush = [&]() {
        if (!line.empty()) {
            out << line << "\n";
            line.clear();
        }
    };
    auto append = [&](const std::string& token) {
        if (!line.empty() && line.size() + 1 + token.size() > 80)
            flush();
        if (!line.empty())
            line += ' ';
        line += token;
    };

    for (const auto& rec : log) {
        if (rec.side == Color::White)
            append(std::to_string(rec.moveNumber) + ".");
        append(rec.san);
    }
    append(pgnResult);
    flush();

    std::cout << "PGN saved to: " << filename << "\n";
}

void EngineMatch::appendSeriesRecord(const std::vector<GameResult>& results) const {
    std::filesystem::create_directories("results");
    const std::string outPath = "results/matches.md";
    const bool isNew = !std::filesystem::exists(outPath);

    std::ofstream out(outPath, std::ios::app);
    if (!out.is_open()) {
        std::cout << "Warning: could not write to " << outPath << "\n";
        return;
    }

    // Build options string
    std::string optStr;
    for (const auto& [k, v] : opponent_.options) {
        if (!optStr.empty())
            optStr += ", ";
        optStr += k + "=" + v;
    }
    if (optStr.empty())
        optStr = "—";

    const std::string tc = std::to_string(timeMs_ / 1000) + "+" + std::to_string(incMs_ / 1000);

    int wins = 0, draws = 0, losses = 0;
    uint64_t totalNps = 0;
    int totalNpsGames = 0;
    int totalDepth = 0, totalDepthMoves = 0;
    double totalTtHit = 0.0, totalTtOcc = 0.0;

    for (const auto& r : results) {
        if (r.aborted)
            continue;
        if (r.score == 1)
            ++wins;
        else if (r.score == 0)
            ++draws;
        else
            ++losses;
        if (r.summary.cchessMoves > 0) {
            totalNps += r.summary.totalNps / static_cast<uint64_t>(r.summary.cchessMoves);
            ++totalNpsGames;
            totalDepth += r.summary.totalDepth;
            totalDepthMoves += r.summary.cchessMoves;
        }
        totalTtHit += r.ttStats.hitRate();
        totalTtOcc += r.ttOccupancy;
    }

    if (isNew) {
        out << "# Engine Match Results\n\n";
        out << "| Date | Opponent | Options | TC | W | D | L |"
               " Avg NPS | Avg Depth | TT Hit | TT Occ |\n";
        out << "|------|----------|---------|----|----|---|---|"
               "---------|-----------|--------|--------|\n";
    }

    int n = static_cast<int>(results.size());
    uint64_t avgNps = totalNpsGames > 0 ? totalNps / static_cast<uint64_t>(totalNpsGames) : 0;
    double avgDepth = totalDepthMoves > 0 ? static_cast<double>(totalDepth) / totalDepthMoves : 0.0;
    double avgTtHit = n > 0 ? totalTtHit / n : 0.0;
    double avgTtOcc = n > 0 ? totalTtOcc / n : 0.0;

    out << std::fixed << std::setprecision(1);
    out << "| " << timestamp("%Y-%m-%d_%H-%M-%S") << " | " << opponent_.name << " | " << optStr
        << " | " << tc << " | " << wins << " | " << draws << " | " << losses << " | " << avgNps
        << " | " << avgDepth << " | " << avgTtHit << "%"
        << " | " << avgTtOcc << "%"
        << " |\n";

    std::cout << "Match record appended to: " << outPath << "\n";
}

}  // namespace cchess
