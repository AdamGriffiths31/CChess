#ifndef CCHESS_POLYGLOT_BOOK_H
#define CCHESS_POLYGLOT_BOOK_H

#include "core/Board.h"
#include "core/Move.h"
#include "core/Position.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cchess {
namespace book {

// Computes the Polyglot Zobrist key for a position using the fixed
// published 781-entry Random64 table. This is a separate key from the
// engine's own TT Zobrist key and must be computed independently.
uint64_t computePolyglotKey(const Position& pos);

// Decodes a raw Polyglot-encoded move (15-bit uint16_t) and validates it
// against the current board's legal moves. Handles:
//   - Polyglot bit layout: bits[2:0]=to-file, [5:3]=to-rank,
//     [8:6]=from-file, [11:9]=from-rank, [14:12]=promotion piece
//   - Castling remap: Polyglot encodes king-to-rook-corner; remapped to
//     the engine's king-to-destination convention before legality check.
// Returns the validated legal Move, or nullopt on hash collision / illegal entry.
std::optional<Move> decodePolyglotMove(uint16_t pgMove, const Board& board);

// A single entry from a Polyglot .bin opening book file.
struct BookEntry {
    uint64_t key;     // Polyglot Zobrist hash of the position
    uint16_t move;    // Encoded move (from/to/promo packed into 15 bits)
    uint16_t weight;  // Move quality: 2*wins + draws (0 = never play)
    uint32_t learn;   // Book learning data (usually 0)
};

// Reads and queries a Polyglot .bin opening book.
class PolyglotBook {
public:
    // Loads the book from a .bin file. Returns true on success.
    // On failure (file not found, bad format) returns false and leaves
    // the book in an unloaded state.
    bool load(const std::string& path);

    // Unloads the book, freeing all entries.
    void unload() { entries_.clear(); }

    bool isLoaded() const { return !entries_.empty(); }

    // Probes the book for the given Polyglot key. Returns a weighted-random
    // Polyglot-encoded move (15-bit uint16_t) if one or more entries exist,
    // or nullopt if no book entry is found.
    std::optional<uint16_t> probe(uint64_t key) const;

private:
    std::vector<BookEntry> entries_;  // sorted by key ascending
};

}  // namespace book
}  // namespace cchess

#endif  // CCHESS_POLYGLOT_BOOK_H
