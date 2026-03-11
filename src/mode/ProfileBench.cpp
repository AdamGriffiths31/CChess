#include "ProfileBench.h"

#include "../ai/Search.h"
#include "../ai/SearchConfig.h"
#include "../ai/TranspositionTable.h"
#include "../core/Board.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace cchess {

namespace {

// Read the first valid FEN from STS1.epd, falling back to Kiwipete.
std::string loadFirstStsPosition() {
    std::string path = "sts/STS1.epd";
    if (std::filesystem::exists(path)) {
        std::ifstream file(path);
        std::string line;
        if (std::getline(file, line) && !line.empty()) {
            // First 4 fields are the FEN parts
            std::istringstream ss(line);
            std::string board, side, castling, ep;
            if (ss >> board >> side >> castling >> ep) {
                return board + " " + side + " " + castling + " " + ep + " 0 1";
            }
        }
    }
    // Fallback: Kiwipete
    return "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
}

}  // namespace

void ProfileBench::run(int searchTimeMs) {
    std::string fen = loadFirstStsPosition();

    std::cout << "=== Profile Bench ===\n";
    std::cout << "FEN:  " << fen << "\n";
    std::cout << "Time: " << searchTimeMs << " ms\n\n";

    Board board(fen);
    TranspositionTable tt;
    auto pawnTable = std::make_unique<eval::PawnTable>();

    SearchConfig config;
    config.searchTime = std::chrono::milliseconds(searchTimeMs);

    auto wallStart = std::chrono::steady_clock::now();

    Search search(board, config, tt, *pawnTable, [](const SearchInfo& info) {
        std::cout << "  depth=" << info.depth << "  score=" << info.score
                  << "  nodes=" << info.nodes << "  time=" << info.timeMs << "ms"
                  << "  nps="
                  << (info.timeMs > 0 ? info.nodes * 1000 / static_cast<uint64_t>(info.timeMs) : 0)
                  << "\n";
    });

    Move best = search.findBestMove();

    auto wallEnd = std::chrono::steady_clock::now();
    int elapsedMs =
        static_cast<int>(std::chrono::duration<double, std::milli>(wallEnd - wallStart).count());

    uint64_t totalNodes = search.totalNodes();
    uint64_t nps = elapsedMs > 0 ? totalNodes * 1000 / static_cast<uint64_t>(elapsedMs) : 0;

    const TTStats& tts = tt.stats();
    std::cout << "\n--- Summary ---\n";
    std::cout << "Best move:   " << best.toAlgebraic() << "\n";
    std::cout << "Total nodes: " << totalNodes << "\n";
    std::cout << "Total time:  " << elapsedMs << " ms\n";
    std::cout << "NPS:         " << nps << "\n";
    std::cout << "TT probes:   " << tts.probes << "\n";
    std::cout << "TT hit rate: " << tts.hitRate() << "%\n";
    std::cout << "TT cutoffs:  " << tts.cutoffRate() << "%\n";
    std::cout << "TT occupancy:" << tt.occupancy() << "%\n";
}

}  // namespace cchess
