#ifndef CCHESS_TRANSPOSITION_TABLE_H
#define CCHESS_TRANSPOSITION_TABLE_H

#include "core/Move.h"
#include "core/Types.h"

#include <cassert>
#include <cstdint>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <immintrin.h>
#endif

namespace cchess {

enum class TTBound : uint8_t { NONE = 0, EXACT = 1, LOWER = 2, UPPER = 3 };

// 10-byte packed TT entry (no padding).
// Layout (ordered to avoid implicit padding):
//   uint16_t hashVerify  — upper 16 bits of Zobrist key
//   int16_t  score       — search score (fits in int16_t for chess)
//   uint16_t move16      — encoded move: from(6)|to(6)|type(3)|promo(1)
//   int16_t  depth       — search depth (int16_t avoids depth-limit and padding)
//   uint8_t  genBound    — generation(6) | bound(2)
//   uint8_t  _pad        — explicit pad byte to reach 10 bytes
// Total: 2+2+2+2+1+1 = 10 bytes, static_assert enforced.
//
// Move encoding in 16 bits:
//   bits 15-10: from square (6 bits, 0-63)
//   bits  9- 4: to square   (6 bits, 0-63)
//   bits  3- 0: typePromo   (4 bits)
//     Non-promotion moves: typePromo = MoveType (0-3, Normal/Capture/EnPassant/Castling)
//     Promotion moves:     typePromo = 4 + promoIndex
//       promoIndex: 0=Queen+Promo, 1=Knight+Promo, 2=Bishop+Promo, 3=Rook+Promo,
//                   4=Queen+PromoCapture, 5=Knight+PromoCapture, 6=Bishop+PromoCapture,
//                   7=Rook+PromoCapture
//     Values 4-11 cover all promotion variants; 0-3 cover non-promotion types.
struct TTEntry {
    uint16_t hashVerify = 0;
    int16_t score = 0;
    uint16_t move16 = 0;
    int16_t depth = 0;
    uint8_t genBound = 0;  // generation(6) | bound(2)
    uint8_t _pad = 0;      // explicit padding to reach 10 bytes

    TTBound bound() const { return static_cast<TTBound>(genBound & 0x3); }
    uint8_t generation() const { return genBound >> 2; }
    bool isEmpty() const { return genBound == 0; }

    // Decode the stored move back to a Move object.
    // Layout: from(6) | to(6) | typePromo(4)
    // typePromo 0-3 = MoveType directly (non-promotion)
    // typePromo 4-7 = Promotion with piece (4=Q,5=N,6=B,7=R)
    // typePromo 8-11 = PromotionCapture with piece (8=Q,9=N,10=B,11=R)
    Move bestMove() const {
        if (move16 == 0)
            return Move{};
        Square from = static_cast<Square>((move16 >> 10) & 0x3F);
        Square to = static_cast<Square>((move16 >> 4) & 0x3F);
        uint8_t typePromo = static_cast<uint8_t>(move16 & 0xF);
        static constexpr PieceType promoTable[4] = {PieceType::Queen, PieceType::Knight,
                                                    PieceType::Bishop, PieceType::Rook};
        if (typePromo >= 8)
            return Move(from, to, MoveType::PromotionCapture, promoTable[typePromo - 8]);
        if (typePromo >= 4)
            return Move(from, to, MoveType::Promotion, promoTable[typePromo - 4]);
        return Move(from, to, static_cast<MoveType>(typePromo));
    }

    // Encode a Move into move16.
    // Layout: from(6) | to(6) | typePromo(4)
    void setMove(const Move& m) {
        if (m.isNull()) {
            move16 = 0;
            return;
        }
        uint16_t typePromo;
        if (m.type() == MoveType::Promotion || m.type() == MoveType::PromotionCapture) {
            uint16_t base = (m.type() == MoveType::PromotionCapture) ? 8u : 4u;
            uint16_t promoIdx;
            switch (m.promotion()) {
                case PieceType::Knight:
                    promoIdx = 1;
                    break;
                case PieceType::Bishop:
                    promoIdx = 2;
                    break;
                case PieceType::Rook:
                    promoIdx = 3;
                    break;
                default:
                    promoIdx = 0;
                    break;  // Queen
            }
            typePromo = base + promoIdx;
        } else {
            typePromo = static_cast<uint16_t>(m.type());
        }
        move16 = static_cast<uint16_t>((static_cast<uint16_t>(m.from()) << 10) |
                                       (static_cast<uint16_t>(m.to()) << 4) | typePromo);
    }
};

static_assert(sizeof(TTEntry) == 10, "TTEntry must be exactly 10 bytes");

// Cluster of 4 entries padded to exactly one 64-byte cache line.
// All 4 candidates are fetched in a single memory transaction.
struct alignas(64) TTCluster {
    TTEntry entries[4];   // 4 × 10 = 40 bytes
    uint8_t padding[24];  // pad to 64 bytes
};
static_assert(sizeof(TTCluster) == 64, "TTCluster must be exactly 64 bytes (one cache line)");

struct TTStats {
    uint64_t probes = 0;
    uint64_t hits = 0;
    uint64_t cutoffs = 0;  // hit with sufficient depth
    uint64_t stores = 0;
    uint64_t overwrites = 0;  // replaced a non-empty slot

    void reset() { probes = hits = cutoffs = stores = overwrites = 0; }
    double hitRate() const {
        return probes ? 100.0 * static_cast<double>(hits) / static_cast<double>(probes) : 0.0;
    }
    double cutoffRate() const {
        return probes ? 100.0 * static_cast<double>(cutoffs) / static_cast<double>(probes) : 0.0;
    }
};

// Mate scores are stored relative to root, not ply. Convert before storing
// and after probing so that mate-in-N is correct regardless of search path.
// The threshold (SCORE_MATE - 200) must match the mate score in Eval.h.
constexpr int TT_MATE_THRESHOLD = 100000 - 200;

inline int scoreToTT(int score, int ply) {
    if (score >= TT_MATE_THRESHOLD)
        return score + ply;
    if (score <= -TT_MATE_THRESHOLD)
        return score - ply;
    return score;
}

inline int scoreFromTT(int score, int ply) {
    if (score >= TT_MATE_THRESHOLD)
        return score - ply;
    if (score <= -TT_MATE_THRESHOLD)
        return score + ply;
    return score;
}

class TranspositionTable {
public:
    explicit TranspositionTable(size_t sizeMB = 128);

    inline bool probe(uint64_t hash, TTEntry& out) {
        ++stats_.probes;
        uint16_t key16 = verifyKey(hash);
        TTCluster& cluster = clusters_[clusterIndex(hash)];
        for (int i = 0; i < 4; ++i) {
            TTEntry& e = cluster.entries[i];
            if (e.hashVerify == key16 && !e.isEmpty()) {
                out = e;
                ++stats_.hits;
                return true;
            }
        }
        return false;
    }

    inline void prefetch(uint64_t hash) const {
        const void* addr = &clusters_[clusterIndex(hash)];
#ifdef _MSC_VER
        _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0);
#else
        __builtin_prefetch(addr, 0, 3);
#endif
    }

    void store(uint64_t hash, int score, int depth, TTBound bound, const Move& bestMove);
    void newSearch();
    void clear();

    size_t clusterCount() const { return mask_ + 1; }
    size_t entryCount() const { return clusterCount() * 4; }
    size_t usedEntries() const;
    double occupancy() const {
        return 100.0 * static_cast<double>(usedEntries()) / static_cast<double>(entryCount());
    }

    TTStats& stats() { return stats_; }
    const TTStats& stats() const { return stats_; }

private:
    // Hash selects cluster index; upper 16 bits stored for entry verification.
    size_t clusterIndex(uint64_t hash) const { return static_cast<size_t>(hash) & mask_; }
    static uint16_t verifyKey(uint64_t hash) { return static_cast<uint16_t>(hash >> 48); }

    std::vector<TTCluster> clusters_;
    size_t mask_ = 0;         // clusterCount - 1 (power of two)
    uint8_t generation_ = 0;  // 6-bit, wraps at 64
    TTStats stats_;
};

}  // namespace cchess

#endif  // CCHESS_TRANSPOSITION_TABLE_H
