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
    REQUIRE(entry.bestMove == move);
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
    REQUIRE(entry.bestMove == move2);
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

TEST_CASE("TT entry size is 16 bytes", "[tt]") {
    REQUIRE(sizeof(TTEntry) == 16);
}

TEST_CASE("TT table size is power of 2", "[tt]") {
    TranspositionTable tt(1);
    size_t count = tt.entryCount();
    REQUIRE((count & (count - 1)) == 0);
}

TEST_CASE("TT generation aging replaces stale entries", "[tt]") {
    TranspositionTable tt(1);
    // Same lower bits (same index), different upper 16 bits (different verify key)
    uint64_t hash1 = 0xAAAA00000000DDDDULL;
    uint64_t hash2 = 0xBBBB00000000DDDDULL;
    Move move1(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_4));
    Move move2(makeSquare(FILE_D, RANK_2), makeSquare(FILE_D, RANK_4));

    // Store deep entry for hash1
    tt.store(hash1, 10, 8, TTBound::EXACT, move1);

    // Advance generation — hash1's entry becomes stale
    tt.newSearch();

    // Shallow entry for hash2 (same index) should replace stale deep entry
    tt.store(hash2, 20, 2, TTBound::LOWER, move2);
    TTEntry entry;
    REQUIRE(tt.probe(hash2, entry));
    REQUIRE(entry.score == 20);
    // hash1 should be gone (replaced)
    REQUIRE_FALSE(tt.probe(hash1, entry));
}
