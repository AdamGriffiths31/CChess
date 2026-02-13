#include "StsRunner.h"

#include "../ai/Search.h"
#include "../ai/SearchConfig.h"
#include "../ai/TranspositionTable.h"
#include "../core/Board.h"
#include "../core/Notation.h"
#include "../core/Square.h"
#include "../utils/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace cchess {

namespace {

// Strip check/checkmate suffixes from a SAN string for matching
std::string stripCheckSuffix(const std::string& san) {
    std::string s = san;
    while (!s.empty() && (s.back() == '+' || s.back() == '#')) {
        s.pop_back();
    }
    return s;
}

// Parse the c0 field: "f5=10, Be5+=2, Bf2=3, Bg4=2" -> map<string, int>
std::map<std::string, int> parseC0(const std::string& c0) {
    std::map<std::string, int> scores;

    // Split on commas
    std::istringstream stream(c0);
    std::string token;
    while (std::getline(stream, token, ',')) {
        // Trim whitespace
        auto start = token.find_first_not_of(' ');
        if (start == std::string::npos)
            continue;
        token = token.substr(start);

        // Find '=' separator (last one, since SAN may contain '=' for promotion)
        auto eq = token.rfind('=');
        if (eq == std::string::npos || eq == 0)
            continue;

        std::string moveSan = token.substr(0, eq);
        std::string scoreStr = token.substr(eq + 1);

        // Trim both parts
        while (!moveSan.empty() && moveSan.back() == ' ')
            moveSan.pop_back();
        auto sStart = scoreStr.find_first_not_of(' ');
        if (sStart != std::string::npos)
            scoreStr = scoreStr.substr(sStart);

        int score = 0;
        try {
            score = std::stoi(scoreStr);
        } catch (...) {
            continue;
        }

        // Store with check suffixes stripped for matching
        scores[stripCheckSuffix(moveSan)] = score;
    }

    return scores;
}

// Parse a single EPD line, returning FEN and c0 scores
// Returns false if the line is malformed or empty
bool parseEpd(const std::string& line, std::string& fen, std::map<std::string, int>& c0Scores) {
    if (line.empty())
        return false;

    // First 4 space-separated fields are the FEN parts
    std::istringstream stream(line);
    std::string board, side, castling, ep;
    if (!(stream >> board >> side >> castling >> ep))
        return false;

    // Build full FEN with default halfmove/fullmove
    fen = board + " " + side + " " + castling + " " + ep + " 0 1";

    // Find c0 field: c0 "...";
    auto c0Pos = line.find("c0 \"");
    if (c0Pos == std::string::npos)
        return false;

    auto c0Start = c0Pos + 4;  // past c0 "
    auto c0End = line.find('"', c0Start);
    if (c0End == std::string::npos)
        return false;

    std::string c0Content = line.substr(c0Start, c0End - c0Start);
    c0Scores = parseC0(c0Content);

    return !c0Scores.empty();
}

std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

}  // namespace

void StsRunner::run() {
    std::cout << "=== STS Benchmark ===\n";

    // Get configuration
    std::cout << "Positions per file (1-100, default 10): ";
    std::string input;
    std::getline(std::cin, input);
    int positionsPerFile = 10;
    if (!input.empty()) {
        try {
            positionsPerFile = std::stoi(input);
            positionsPerFile = std::max(1, std::min(100, positionsPerFile));
        } catch (...) {
            positionsPerFile = 10;
        }
    }

    std::cout << "Search time per position in ms (default 5000): ";
    std::getline(std::cin, input);
    int searchTimeMs = 5000;
    if (!input.empty()) {
        try {
            searchTimeMs = std::stoi(input);
            searchTimeMs = std::max(100, searchTimeMs);
        } catch (...) {
            searchTimeMs = 5000;
        }
    }

    SearchConfig config;
    config.searchTime = std::chrono::milliseconds(searchTimeMs);

    // Find STS files
    std::vector<std::string> stsFiles;
    for (int i = 1; i <= 15; ++i) {
        std::string path = "sts/STS" + std::to_string(i) + ".epd";
        if (std::filesystem::exists(path)) {
            stsFiles.push_back(path);
        }
    }

    if (stsFiles.empty()) {
        std::cout << "No STS files found in sts/ directory.\n";
        return;
    }

    std::sort(stsFiles.begin(), stsFiles.end());

    std::cout << "\nFound " << stsFiles.size() << " STS file(s). "
              << "Running " << positionsPerFile << " positions each at " << searchTimeMs
              << "ms/position.\n\n";

    // Results storage
    struct FileResult {
        std::string filename;
        int score;
        int maxScore;
    };
    std::vector<FileResult> results;
    int totalScore = 0;
    int totalMax = 0;

    for (const auto& filepath : stsFiles) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "Failed to open: " << filepath << "\n";
            continue;
        }

        // Extract filename for display
        std::string filename = std::filesystem::path(filepath).filename().string();
        std::cout << "--- " << filename << " ---\n";

        int fileScore = 0;
        int fileMax = 0;
        int posCount = 0;
        std::string line;

        while (std::getline(file, line) && posCount < positionsPerFile) {
            std::string fen;
            std::map<std::string, int> c0Scores;

            if (!parseEpd(line, fen, c0Scores))
                continue;

            Board board(fen);
            TranspositionTable tt;
            Search search(board, config, tt);
            Move bestMove = search.findBestMove();

            if (bestMove.isNull()) {
                std::cout << "  #" << (posCount + 1) << ": no move found\n";
                ++posCount;
                fileMax += 10;
                continue;
            }

            std::string san = moveToSan(board, bestMove);
            std::string sanStripped = stripCheckSuffix(san);

            int score = 0;
            auto it = c0Scores.find(sanStripped);
            if (it != c0Scores.end()) {
                score = it->second;
            }

            fileScore += score;
            fileMax += 10;
            ++posCount;

            // Find the best-scoring move(s) from c0
            std::string bestMove_san;
            int bestScore = 0;
            for (const auto& [mv, sc] : c0Scores) {
                if (sc > bestScore) {
                    bestScore = sc;
                    bestMove_san = mv;
                }
            }

            std::cout << "  #" << posCount << ": " << san << " (" << score << "/10)"
                      << "  expected: " << bestMove_san << "\n";
        }

        std::cout << "  Score: " << fileScore << "/" << fileMax << "\n\n";

        results.push_back({filename, fileScore, fileMax});
        totalScore += fileScore;
        totalMax += fileMax;
    }

    // Summary
    std::cout << "=== Total: " << totalScore << "/" << totalMax << " ===\n";

    // Append results to results/sts.md as a new table row
    std::filesystem::create_directories("results");
    std::string outPath = "results/sts.md";
    bool fileExists = std::filesystem::exists(outPath);
    std::ofstream out(outPath, std::ios::app);
    if (out.is_open()) {
        if (!fileExists) {
            out << "# STS Benchmark Results\n\n";
            out << "| Date | Time (ms) | Positions |";
            for (const auto& r : results) {
                out << " " << r.filename << " |";
            }
            out << " Total | % |\n";
            out << "|------|-----------|-----------|";
            for (size_t i = 0; i < results.size(); ++i) {
                out << "------|";
            }
            out << "-------|---|\n";
        }
        double totalPct = totalMax > 0 ? 100.0 * totalScore / totalMax : 0.0;
        out << "| " << generateTimestamp() << " | " << searchTimeMs << " | " << positionsPerFile
            << " |";
        for (const auto& r : results) {
            out << " " << r.score << "/" << r.maxScore << " |";
        }
        out << " " << totalScore << "/" << totalMax << " | " << std::fixed << std::setprecision(1)
            << totalPct << "% |\n";
        std::cout << "Results appended to: " << outPath << "\n";
    } else {
        std::cout << "Warning: could not write results file.\n";
    }
}

}  // namespace cchess
