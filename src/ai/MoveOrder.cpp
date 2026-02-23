#include "ai/MoveOrder.h"

#include <algorithm>
#include <cassert>

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

// Score bands (higher = searched earlier):
//   TT move         : 1,000,000  (handled via score injection in the killer overload)
//   Good captures   : ~100..8900  (MVV-LVA: victim*10 - attacker; range ~-900..9000)
//   Promotions      : MVV_LVA_VALUE[promo]*10 added (e.g. queen promo = +9000)
//   Killer 1        : 8000  (above most captures except queen promotions)
//   Killer 2        : 7000
//   Losing captures : can be negative (e.g. queen takes pawn = 100 - 900 = -800)
//   Other quiets    : 0
//
// Note: killers at 7000/8000 interleave with the capture range (~-900..9000),
// sitting above losing captures but below good captures.
constexpr int KILLER1_SCORE = 8000;
constexpr int KILLER2_SCORE = 7000;

int MoveOrder::score(const Move& move, const Position& pos) {
    int s = 0;

    // Victim value - attacker value (captures)
    if (move.isCapture()) {
        if (move.isEnPassant()) {
            s = MVV_LVA_VALUE[static_cast<int>(PieceType::Pawn)] * 10;
        } else {
            assert(!pos.pieceAt(move.to()).isEmpty());
            assert(!pos.pieceAt(move.from()).isEmpty());
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
    assert(moves.size() <= 256);
    // Compute scores once upfront
    int scores[256];
    for (size_t i = 0; i < moves.size(); ++i)
        scores[i] = score(moves[i], pos);

    // Insertion sort -- good for the small lists typical in chess (~30-40 moves)
    for (size_t i = 1; i < moves.size(); ++i) {
        Move key = moves[i];
        int keyScore = scores[i];
        size_t j = i;
        while (j > 0 && scores[j - 1] < keyScore) {
            moves[j] = moves[j - 1];
            scores[j] = scores[j - 1];
            --j;
        }
        moves[j] = key;
        scores[j] = keyScore;
    }
}

static bool matchesTTMove(const Move& move, const Move& ttMove) {
    return !ttMove.isNull() && move == ttMove;
}

void MoveOrder::sort(MoveList& moves, const Position& pos, const Move& ttMove) {
    assert(moves.size() <= 256);
    // Put TT move first, then sort the rest normally
    for (size_t i = 0; i < moves.size(); ++i) {
        if (matchesTTMove(moves[i], ttMove)) {
            // Swap to front
            Move tmp = moves[0];
            moves[0] = moves[i];
            moves[i] = tmp;

            // Compute scores once for the rest (index 1..end)
            int scores[256];
            for (size_t j = 1; j < moves.size(); ++j)
                scores[j] = score(moves[j], pos);

            // Sort the rest (index 1..end)
            for (size_t j = 2; j < moves.size(); ++j) {
                Move key = moves[j];
                int keyScore = scores[j];
                size_t k = j;
                while (k > 1 && scores[k - 1] < keyScore) {
                    moves[k] = moves[k - 1];
                    scores[k] = scores[k - 1];
                    --k;
                }
                moves[k] = key;
                scores[k] = keyScore;
            }
            return;
        }
    }
    // TT move not found in list, just sort normally
    sort(moves, pos);
}

void MoveOrder::sort(MoveList& moves, const Position& pos, const Move& ttMove,
                     const Move* killers) {
    assert(moves.size() <= 256);

    // Compute scores, applying killer bonuses to quiet moves
    int scores[256];
    for (size_t i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        if (matchesTTMove(m, ttMove)) {
            scores[i] = 1'000'000;  // Always first
        } else {
            scores[i] = score(m, pos);
            // Killer bonus only for quiet non-promotion moves
            if (!m.isCapture() && !m.isPromotion()) {
                if (!killers[0].isNull() && m == killers[0])
                    scores[i] = KILLER1_SCORE;
                else if (!killers[1].isNull() && m == killers[1])
                    scores[i] = KILLER2_SCORE;
            }
        }
    }

    for (size_t i = 1; i < moves.size(); ++i) {
        Move key = moves[i];
        int keyScore = scores[i];
        size_t j = i;
        while (j > 0 && scores[j - 1] < keyScore) {
            moves[j] = moves[j - 1];
            scores[j] = scores[j - 1];
            --j;
        }
        moves[j] = key;
        scores[j] = keyScore;
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
