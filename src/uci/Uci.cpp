#include "uci/Uci.h"

#include "ai/Eval.h"
#include "ai/Search.h"
#include "ai/SearchConfig.h"
#include "book/PolyglotBook.h"
#include "core/Move.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

namespace cchess {

void Uci::loop() {
    // Disable stdout buffering for UCI protocol
    std::cout.setf(std::ios::unitbuf);

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci")
            handleUci();
        else if (cmd == "isready")
            handleIsReady();
        else if (cmd == "ucinewgame")
            handleNewGame();
        else if (cmd == "position")
            handlePosition(iss);
        else if (cmd == "go")
            handleGo(iss);
        else if (cmd == "stop")
            handleStop();
        else if (cmd == "setoption")
            handleSetOption(iss);
        else if (cmd == "eval")
            handleEval();
        else if (cmd == "quit") {
            handleStop();
            return;
        }
    }

    // EOF reached — wait for search to finish naturally (don't force stop)
    joinSearch();
}

void Uci::handleUci() {
    std::cout << "id name CChess\n";
    std::cout << "id author Adam\n";
    std::cout << "option name OwnBook type check default false\n";
    std::cout << "option name BookFile type string default engines/book.bin\n";
    std::cout << "uciok\n";
}

void Uci::handleIsReady() {
    joinSearch();
    std::cout << "readyok\n";
}

void Uci::handleNewGame() {
    joinSearch();
    tt_.clear();
    board_ = Board();
    gameHistory_.clear();
}

void Uci::handlePosition(std::istringstream& args) {
    joinSearch();

    std::string token;
    args >> token;

    gameHistory_.clear();

    if (token == "startpos") {
        board_ = Board();
        args >> token;  // consume "moves" if present
    } else if (token == "fen") {
        std::string fen;
        // FEN has 6 space-separated fields
        for (int i = 0; i < 6 && (args >> token); ++i) {
            if (i > 0)
                fen += ' ';
            fen += token;
        }
        board_ = Board(fen);
        args >> token;  // consume "moves" if present
    }

    // Apply moves, recording hash before each move for repetition detection
    while (args >> token) {
        if (token == "moves")
            continue;

        auto parsed = Move::fromAlgebraic(token);
        if (!parsed)
            continue;

        PieceType promo = parsed->isPromotion() ? parsed->promotion() : PieceType::None;
        auto legal = board_.findLegalMove(parsed->from(), parsed->to(), promo);
        if (legal) {
            gameHistory_.push_back(board_.position().hash());
            board_.makeMoveUnchecked(*legal);
        }
    }
}

void Uci::handleGo(std::istringstream& args) {
    joinSearch();

    // Opening book probe — only within the first bookDepth_ moves
    if (useOwnBook_ && book_.isLoaded()) {
        // fullmoveNumber is 1-based and increments after Black's move.
        // Half-moves played = (fullmoveNumber - 1) * 2 + (Black to move ? 1 : 0)
        int halfMoves =
            (board_.fullmoveNumber() - 1) * 2 + (board_.sideToMove() == Color::Black ? 1 : 0);
        if (halfMoves < bookDepth_ * 2) {
            uint64_t pgKey = book::computePolyglotKey(board_.position());
            if (auto pgMove = book_.probe(pgKey)) {
                if (auto legal = book::decodePolyglotMove(*pgMove, board_)) {
                    std::cout << "info string book move\n";
                    std::cout << "bestmove " << legal->toAlgebraic() << "\n";
                    return;
                }
                // Illegal move from book — fall through to normal search
            }
        }
    }

    std::string token;
    int wtime = -1, btime = -1, winc = 0, binc = 0;
    int depth = -1, movetime = -1;
    bool infinite = false;

    while (args >> token) {
        if (token == "wtime")
            args >> wtime;
        else if (token == "btime")
            args >> btime;
        else if (token == "winc")
            args >> winc;
        else if (token == "binc")
            args >> binc;
        else if (token == "depth")
            args >> depth;
        else if (token == "movetime")
            args >> movetime;
        else if (token == "infinite")
            infinite = true;
    }

    SearchConfig config;
    config.stopSignal = &stopFlag_;
    stopFlag_.store(false);

    if (depth > 0) {
        config.maxDepth = depth;
        config.searchTime = std::chrono::milliseconds(300000);  // 5 min cap
    } else if (movetime > 0) {
        config.searchTime = std::chrono::milliseconds(movetime);
    } else if (infinite) {
        config.maxDepth = 64;
        config.searchTime = std::chrono::milliseconds(300000);
    } else {
        // Time management: allocate from remaining time
        int remaining = (board_.sideToMove() == Color::White) ? wtime : btime;
        int increment = (board_.sideToMove() == Color::White) ? winc : binc;

        if (remaining > 0) {
            int allocated = remaining / 30 + increment;
            allocated = std::min(allocated, remaining / 3);
            // Never spend less than increment (minus a small overhead margin)
            if (increment > 0)
                allocated = std::max(allocated, increment - 50);
            config.searchTime = std::chrono::milliseconds(allocated);
        }
    }

    // Info callback for UCI info lines
    auto infoCallback = [](const SearchInfo& info) {
        std::cout << "info depth " << info.depth;

        // Score: mate or centipawns
        if (info.score >= eval::SCORE_MATE - 200) {
            int matePly = eval::SCORE_MATE - info.score;
            int mateN = (matePly + 1) / 2;
            std::cout << " score mate " << mateN;
        } else if (info.score <= -(eval::SCORE_MATE - 200)) {
            int matePly = eval::SCORE_MATE + info.score;
            int mateN = (matePly + 1) / 2;
            std::cout << " score mate -" << mateN;
        } else {
            std::cout << " score cp " << info.score;
        }

        std::cout << " nodes " << info.nodes;

        int timeMs = std::max(info.timeMs, 1);
        uint64_t nps = info.nodes * 1000 / static_cast<uint64_t>(timeMs);
        std::cout << " nps " << nps;
        std::cout << " time " << info.timeMs;

        if (!info.pv.empty()) {
            std::cout << " pv";
            for (const auto& m : info.pv) {
                std::cout << " " << m.toAlgebraic();
            }
        }

        std::cout << "\n";
    };

    // Launch search in background thread
    Board boardCopy = board_;
    std::vector<uint64_t> historyCopy = gameHistory_;
    searchThread_ = std::thread(
        [this, config, infoCallback, boardCopy, historyCopy = std::move(historyCopy)]() mutable {
            Search search(boardCopy, config, tt_, pawnTable_, infoCallback, std::move(historyCopy));
            Move best = search.findBestMove();
            std::cout << "bestmove " << best.toAlgebraic() << "\n";
        });
}

void Uci::handleSetOption(std::istringstream& args) {
    // Expected format: setoption name <Name> value <Value>
    std::string token, name, value;

    args >> token;  // "name"
    if (token != "name")
        return;

    // Name may be multiple words; read until "value" or end
    while (args >> token && token != "value") {
        if (!name.empty())
            name += ' ';
        name += token;
    }

    // Value may be multiple words; read remainder
    if (token == "value") {
        while (args >> token) {
            if (!value.empty())
                value += ' ';
            value += token;
        }
    }

    if (name == "OwnBook") {
        useOwnBook_ = (value == "true");
    } else if (name == "BookFile") {
        if (value.empty()) {
            book_.unload();
        } else if (!book_.load(value)) {
            std::cerr << "info string Warning: could not load book file: " << value << "\n";
        }
    }
}

void Uci::handleEval() {
    const Position& pos = board_.position();
    Bitboard wp = pos.pieces(PieceType::Pawn, Color::White);
    Bitboard bp = pos.pieces(PieceType::Pawn, Color::Black);

    eval::EvalState state;
    eval::Score mat = pos.psqt();
    eval::Score bpair = eval::bishopPair(pos);
    eval::Score pawn = eval::pawnStructure(wp, bp);
    eval::Score pass = eval::passedPawns(wp, bp);
    eval::Score rook = eval::rookOpenFiles(pos, wp, bp);
    eval::Score piece = eval::pieceEval(pos, wp, bp, state);
    eval::Score ksafe = eval::kingSafety(pos, wp, bp, state);

    eval::Score total = mat + bpair + pawn + pass + rook + piece + ksafe;

    int phase = eval::gamePhase(pos);  // not exposed — compute inline
    // Re-use total which is already summed
    int tapered = (total.mg * phase + total.eg * (eval::TOTAL_PHASE - phase)) / eval::TOTAL_PHASE;
    int stm = (pos.sideToMove() == Color::White) ? tapered : -tapered;

    auto row = [](const char* name, eval::Score s) {
        std::cout << "  " << std::left << std::setw(20) << name << " MG: " << std::setw(6) << s.mg
                  << " EG: " << s.eg << "\n";
    };

    std::cout << "info string --- Eval breakdown (White-relative cp) ---\n";
    row("Material+PST", mat);
    row("Bishop pair", bpair);
    row("Pawn structure", pawn);
    row("Passed pawns", pass);
    row("Rook open files", rook);
    row("Piece/mobility", piece);
    row("King safety", ksafe);
    row("TOTAL", total);
    std::cout << "info string Phase: " << phase << "/" << eval::TOTAL_PHASE << "\n";
    std::cout << "info string Tapered (White-relative): " << tapered << " cp\n";
    std::cout << "info string Final (side-to-move): " << stm << " cp\n";
}

void Uci::handleStop() {
    stopFlag_.store(true);
    joinSearch();
}

void Uci::joinSearch() {
    if (searchThread_.joinable())
        searchThread_.join();
}

}  // namespace cchess
