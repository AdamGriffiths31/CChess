#include "ai/MoveOrder.h"

#include <algorithm>

namespace cchess {

// Piece values for MVV-LVA scoring (indexed by PieceType)
constexpr int MVV_LVA_VALUE[] = {
    100,  // Pawn
    300,  // Knight
    300,  // Bishop
    500,  // Rook
    900,  // Queen
    0     // King
};

int MoveOrder::score(const Move& move, const Position& pos) {
    int s = 0;

    // Victim value - attacker value (captures)
    if (move.isCapture()) {
        if (move.isEnPassant()) {
            s = MVV_LVA_VALUE[static_cast<int>(PieceType::Pawn)] * 10;
        } else {
            PieceType victim = pos.pieceAt(move.to()).type();
            PieceType attacker = pos.pieceAt(move.from()).type();
            s = MVV_LVA_VALUE[static_cast<int>(victim)] * 10 -
                MVV_LVA_VALUE[static_cast<int>(attacker)];
        }
    }

    // Bonus for promotions
    if (move.isPromotion()) {
        s += MVV_LVA_VALUE[static_cast<int>(move.promotion())] * 10;
    }

    return s;
}

void MoveOrder::sort(MoveList& moves, const Position& pos) {
    // Insertion sort -- good for the small lists typical in chess (~30-40 moves)
    for (size_t i = 1; i < moves.size(); ++i) {
        Move key = moves[i];
        int keyScore = score(key, pos);
        size_t j = i;
        while (j > 0 && score(moves[j - 1], pos) < keyScore) {
            moves[j] = moves[j - 1];
            --j;
        }
        moves[j] = key;
    }
}

size_t MoveOrder::extractCaptures(const MoveList& moves, const Position& pos, Move* out,
                                  size_t maxOut) {
    struct ScoredMove {
        Move move;
        int score;
    };
    ScoredMove buf[256];
    size_t n = 0;

    for (size_t i = 0; i < moves.size() && n < maxOut; ++i) {
        const Move& m = moves[i];
        if (m.isCapture() || m.isPromotion()) {
            buf[n++] = {m, score(m, pos)};
        }
    }

    std::sort(buf, buf + n,
              [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    for (size_t i = 0; i < n; ++i)
        out[i] = buf[i].move;

    return n;
}

}  // namespace cchess
