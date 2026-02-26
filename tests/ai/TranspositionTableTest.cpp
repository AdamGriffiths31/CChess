#include "ai/TranspositionTable.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("TT probe misses on empty table", "[tt]") {
    TranspositionTable tt(1);  // 1 MB
    TTEntry entry;
    REQUIRE_FALSE(tt.probe(0x123456789ABCDEF0ULL, entry));
}

TEST_CASE("TT store then probe returns matching entry", "[tt]") {
    TranspositionTable tt(1);
    uint64_t hash = 0xDEADBEEFCAFEBABEULL;
    Move move(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_4));

    tt.store(hash, 42, 5, TTBound::EXACT, move);

    TTEntry entry;
    REQUIRE(tt.probe(hash, entry));
    REQUIRE(entry.score == 42);
    REQUIRE(entry.depth == 5);
    REQUIRE(entry.bound() == TTBound::EXACT);
    REQUIRE(entry.bestMove() == move);
}

TEST_CASE("TT always-replace overwrites", "[tt]") {
    TranspositionTable tt(1);
    uint64_t hash = 0x1111111111111111ULL;
    Move move1(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_4));
    Move move2(makeSquare(FILE_D, RANK_2), makeSquare(FILE_D, RANK_4));

    tt.store(hash, 10, 3, TTBound::LOWER, move1);
    tt.store(hash, 20, 5, TTBound::UPPER, move2);

    TTEntry entry;
    REQUIRE(tt.probe(hash, entry));
    REQUIRE(entry.score == 20);
    REQUIRE(entry.depth == 5);
    REQUIRE(entry.bound() == TTBound::UPPER);
    REQUIRE(entry.bestMove() == move2);
}

TEST_CASE("TT clear resets all entries", "[tt]") {
    TranspositionTable tt(1);
    uint64_t hash = 0xAAAAAAAAAAAAAAAAULL;
    Move move(makeSquare(FILE_A, RANK_1), makeSquare(FILE_A, RANK_2));

    tt.store(hash, 99, 7, TTBound::EXACT, move);

    TTEntry entry;
    REQUIRE(tt.probe(hash, entry));

    tt.clear();
    REQUIRE_FALSE(tt.probe(hash, entry));
}

TEST_CASE("TT different hashes don't collide", "[tt]") {
    TranspositionTable tt(1);
    // Hashes differ in both lower bits (index) and upper bits (verify key)
    uint64_t hash1 = 0x1000000000000001ULL;
    uint64_t hash2 = 0x2000000000000002ULL;
    Move move1(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_4));
    Move move2(makeSquare(FILE_D, RANK_2), makeSquare(FILE_D, RANK_4));

    tt.store(hash1, 10, 3, TTBound::EXACT, move1);
    tt.store(hash2, 20, 5, TTBound::EXACT, move2);

    TTEntry entry;
    REQUIRE(tt.probe(hash1, entry));
    REQUIRE(entry.score == 10);
    REQUIRE(tt.probe(hash2, entry));
    REQUIRE(entry.score == 20);
}

TEST_CASE("TT entry size is 10 bytes", "[tt]") {
    REQUIRE(sizeof(TTEntry) == 10);
}

TEST_CASE("TT cluster size is 64 bytes", "[tt]") {
    REQUIRE(sizeof(TTCluster) == 64);
}

TEST_CASE("TT table size is power of 2", "[tt]") {
    TranspositionTable tt(1);
    size_t count = tt.entryCount();
    REQUIRE((count & (count - 1)) == 0);
}

TEST_CASE("TT generation aging replaces stale entries when cluster is full", "[tt]") {
    TranspositionTable tt(1);
    // All 5 hashes map to the same cluster index (same lower bits), different verify keys.
    // Fill all 4 slots with stale entries, then store a fresh one — the stalest must be evicted.
    uint64_t base = 0x0000000000000001ULL;  // lower bits select cluster
    uint64_t hashes[5];
    for (int i = 0; i < 5; ++i)
        hashes[i] = base | (static_cast<uint64_t>(i + 1) << 48);  // vary upper 16 bits

    Move moves[5];
    moves[0] = Move(makeSquare(FILE_A, RANK_1), makeSquare(FILE_A, RANK_2));
    moves[1] = Move(makeSquare(FILE_B, RANK_1), makeSquare(FILE_B, RANK_2));
    moves[2] = Move(makeSquare(FILE_C, RANK_1), makeSquare(FILE_C, RANK_2));
    moves[3] = Move(makeSquare(FILE_D, RANK_1), makeSquare(FILE_D, RANK_2));
    moves[4] = Move(makeSquare(FILE_E, RANK_1), makeSquare(FILE_E, RANK_2));

    // Fill all 4 cluster slots with shallow entries from generation 0
    for (int i = 0; i < 4; ++i)
        tt.store(hashes[i], i * 10, 1, TTBound::LOWER, moves[i]);

    // Advance generation — all 4 entries become stale
    tt.newSearch();

    // Store a 5th entry: should evict one of the stale entries
    tt.store(hashes[4], 99, 3, TTBound::EXACT, moves[4]);

    // The new entry must be retrievable
    TTEntry entry;
    REQUIRE(tt.probe(hashes[4], entry));
    REQUIRE(entry.score == 99);
}
