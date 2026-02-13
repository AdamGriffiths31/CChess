#include "core/Board.h"
#include "core/Position.h"
#include "core/Zobrist.h"
#include "fen/FenParser.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("Zobrist keys are non-zero", "[zobrist]") {
    // Spot-check that keys are populated (non-zero)
    REQUIRE(zobrist::sideKey != 0);
    REQUIRE(zobrist::pieceKeys[0][0][0] != 0);
    REQUIRE(zobrist::pieceKeys[1][5][63] != 0);
    REQUIRE(zobrist::castlingKeys[1] != 0);
    REQUIRE(zobrist::enPassantKeys[0] != 0);
}

TEST_CASE("Zobrist piece keys are distinct", "[zobrist]") {
    // No two piece keys should be identical
    for (int c = 0; c < 2; ++c)
        for (int pt = 0; pt < 6; ++pt)
            for (int sq = 0; sq < 64; ++sq)
                for (int c2 = c; c2 < 2; ++c2)
                    for (int pt2 = (c2 == c ? pt : 0); pt2 < 6; ++pt2)
                        for (int sq2 = (c2 == c && pt2 == pt ? sq + 1 : 0); sq2 < 64; ++sq2)
                            REQUIRE(zobrist::pieceKeys[c][pt][sq] !=
                                    zobrist::pieceKeys[c2][pt2][sq2]);
}

TEST_CASE("computeHash matches incremental after makeMove/unmakeMove", "[zobrist]") {
    Board board;  // starting position

    MoveList moves = board.getLegalMoves();
    REQUIRE(moves.size() == 20);

    for (size_t i = 0; i < moves.size(); ++i) {
        uint64_t hashBefore = board.position().hash();

        UndoInfo undo = board.makeMoveUnchecked(moves[i]);

        // Verify incremental hash matches full recompute
        Position copy = board.position();
        copy.computeHash();
        REQUIRE(board.position().hash() == copy.hash());

        board.unmakeMove(moves[i], undo);

        // Hash restored after unmake
        REQUIRE(board.position().hash() == hashBefore);
    }
}

TEST_CASE("Hash changes with side to move", "[zobrist]") {
    Position pos = FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Position pos2 = FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    REQUIRE(pos.hash() != pos2.hash());
}

TEST_CASE("Hash changes with castling rights", "[zobrist]") {
    Position pos = FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Position pos2 = FenParser::parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kq - 0 1");
    REQUIRE(pos.hash() != pos2.hash());
}

TEST_CASE("Hash changes with en passant file", "[zobrist]") {
    Position pos = FenParser::parse("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    Position pos2 = FenParser::parse("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    REQUIRE(pos.hash() != pos2.hash());
}

TEST_CASE("Same position via different move orders gives same hash", "[zobrist]") {
    // 1. e3 d6 2. d3
    Board board1;
    board1.makeMove(Move(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_3)));  // e2e3
    board1.makeMove(Move(makeSquare(FILE_D, RANK_7), makeSquare(FILE_D, RANK_6)));  // d7d6
    board1.makeMove(Move(makeSquare(FILE_D, RANK_2), makeSquare(FILE_D, RANK_3)));  // d2d3

    // 1. d3 d6 2. e3
    Board board2;
    board2.makeMove(Move(makeSquare(FILE_D, RANK_2), makeSquare(FILE_D, RANK_3)));  // d2d3
    board2.makeMove(Move(makeSquare(FILE_D, RANK_7), makeSquare(FILE_D, RANK_6)));  // d7d6
    board2.makeMove(Move(makeSquare(FILE_E, RANK_2), makeSquare(FILE_E, RANK_3)));  // e2e3

    REQUIRE(board1.position().hash() == board2.position().hash());
}

TEST_CASE("Incremental hash correct for castling", "[zobrist]") {
    // Position where white can castle kingside
    Board board("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    Move castle = Move::makeCastling(makeSquare(FILE_E, RANK_1), makeSquare(FILE_G, RANK_1));
    UndoInfo undo = board.makeMoveUnchecked(castle);

    Position copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());

    board.unmakeMove(castle, undo);
    copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());
}

TEST_CASE("Incremental hash correct for en passant", "[zobrist]") {
    Board board("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1");
    Move ep = Move::makeEnPassant(makeSquare(FILE_E, RANK_5), makeSquare(FILE_D, RANK_6));
    UndoInfo undo = board.makeMoveUnchecked(ep);

    Position copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());

    board.unmakeMove(ep, undo);
    copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());
}

TEST_CASE("Incremental hash correct for promotion", "[zobrist]") {
    Board board("8/P7/8/8/8/8/1k6/K7 w - - 0 1");
    Move promo = Move::makePromotion(makeSquare(FILE_A, RANK_7), makeSquare(FILE_A, RANK_8),
                                     PieceType::Queen);
    UndoInfo undo = board.makeMoveUnchecked(promo);

    Position copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());

    board.unmakeMove(promo, undo);
    copy = board.position();
    copy.computeHash();
    REQUIRE(board.position().hash() == copy.hash());
}
