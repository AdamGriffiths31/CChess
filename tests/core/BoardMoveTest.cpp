#include "core/Board.h"
#include "core/Move.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

TEST_CASE("Board makeMove validates illegal moves", "[board][move]") {
    Board board;

    SECTION("Reject illegal pawn move") {
        // Try to move pawn 3 squares forward
        Move illegalMove(8, 32, MoveType::Normal);  // a2-a5
        REQUIRE_FALSE(board.makeMove(illegalMove));
    }

    SECTION("Reject move of opponent's piece") {
        // White to move, try to move black pawn
        Move illegalMove(48, 40, MoveType::Normal);  // a7-a6
        REQUIRE_FALSE(board.makeMove(illegalMove));
    }

    SECTION("Reject moving into check") {
        Board board2("rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 2");
        // Black queen can capture on g4, putting white king in check isn't the issue
        // But if white tries to move king into attack...
        // Let's use a simpler position
        Board board3("4k3/8/8/8/8/8/8/R3K2r w Q - 0 1");
        // White king on e1, try to move to f1 where black rook attacks
        Move illegalMove(4, 5, MoveType::Normal);  // e1-f1
        REQUIRE_FALSE(board3.makeMove(illegalMove));
    }
}

TEST_CASE("Board makeMove executes legal moves", "[board][move]") {
    Board board;

    SECTION("Normal pawn move") {
        Move move(12, 28, MoveType::Normal);  // e2-e4
        REQUIRE(board.makeMove(move));

        // Check the piece moved
        REQUIRE(board.at(28).type() == PieceType::Pawn);
        REQUIRE(board.at(28).color() == Color::White);
        REQUIRE(board.at(12).isEmpty());
    }

    SECTION("Pawn capture") {
        Board board2("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
        Move move(28, 35, MoveType::Capture);  // e4xd5
        REQUIRE(board2.makeMove(move));

        REQUIRE(board2.at(35).type() == PieceType::Pawn);
        REQUIRE(board2.at(35).color() == Color::White);
        REQUIRE(board2.at(28).isEmpty());
    }
}

TEST_CASE("Board side to move switching", "[board][move]") {
    Board board;

    REQUIRE(board.sideToMove() == Color::White);

    Move move(12, 28, MoveType::Normal);  // e2-e4
    board.makeMove(move);

    REQUIRE(board.sideToMove() == Color::Black);

    Move move2(52, 36, MoveType::Normal);  // e7-e5
    board.makeMove(move2);

    REQUIRE(board.sideToMove() == Color::White);
}

TEST_CASE("Board halfmove clock updates", "[board][move]") {
    Board board;

    SECTION("Reset on pawn move") {
        Board board2("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5 1");
        REQUIRE(board2.halfmoveClock() == 5);

        Move move(12, 28, MoveType::Normal);  // e2-e4
        board2.makeMove(move);

        REQUIRE(board2.halfmoveClock() == 0);
    }

    SECTION("Reset on capture") {
        Board board2("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 5 2");
        REQUIRE(board2.halfmoveClock() == 5);

        Move move(28, 35, MoveType::Capture);  // e4xd5
        board2.makeMove(move);

        REQUIRE(board2.halfmoveClock() == 0);
    }

    SECTION("Increment on other moves") {
        Board board2("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");

        Move move(57, 42, MoveType::Normal);  // Nf6
        board2.makeMove(move);

        REQUIRE(board2.halfmoveClock() == 1);
    }
}

TEST_CASE("Board fullmove number updates", "[board][move]") {
    Board board;

    REQUIRE(board.fullmoveNumber() == 1);

    // White's move
    Move move1(12, 28, MoveType::Normal);  // e2-e4
    board.makeMove(move1);
    REQUIRE(board.fullmoveNumber() == 1);  // Still 1 after white's move

    // Black's move
    Move move2(52, 36, MoveType::Normal);  // e7-e5
    board.makeMove(move2);
    REQUIRE(board.fullmoveNumber() == 2);  // Increments after black's move
}

TEST_CASE("Board castling rights removal", "[board][move]") {
    Board board("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");

    SECTION("King move removes both rights") {
        REQUIRE(board.castlingRights() == ALL_CASTLING);

        Move move(4, 5, MoveType::Normal);  // Ke2
        board.makeMove(move);

        REQUIRE((board.castlingRights() & WHITE_CASTLING) == NO_CASTLING);
        REQUIRE((board.castlingRights() & BLACK_CASTLING) != NO_CASTLING);
    }

    SECTION("Queenside rook move removes queenside right") {
        Move move(0, 1, MoveType::Normal);  // Rb1
        board.makeMove(move);

        REQUIRE((board.castlingRights() & WHITE_QUEENSIDE) == NO_CASTLING);
        REQUIRE((board.castlingRights() & WHITE_KINGSIDE) != NO_CASTLING);
    }

    SECTION("Kingside rook move removes kingside right") {
        Move move(7, 6, MoveType::Normal);  // Rg1
        board.makeMove(move);

        REQUIRE((board.castlingRights() & WHITE_KINGSIDE) == NO_CASTLING);
        REQUIRE((board.castlingRights() & WHITE_QUEENSIDE) != NO_CASTLING);
    }

    SECTION("Rook capture removes opponent's castling right") {
        // Set up position where white can capture black's rook
        Board board2("r3k1br/ppppppp1/6p1/8/8/6P1/PPPPPPP1/3RK2R w Kkq - 0 1");
        Move move(7, 63, MoveType::Capture);  // Rxh8 (capture black's rook)
        board2.makeMove(move);

        REQUIRE((board2.castlingRights() & BLACK_KINGSIDE) == NO_CASTLING);
    }
}

TEST_CASE("Board en passant square calculation", "[board][move]") {
    Board board;

    SECTION("Set on double pawn push") {
        Move move(12, 28, MoveType::Normal);  // e2-e4
        board.makeMove(move);

        REQUIRE(board.enPassantSquare() == 20);  // e3
    }

    SECTION("Clear on other moves") {
        Board board2("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        REQUIRE(board2.enPassantSquare() == 20);  // e3

        Move move(57, 42, MoveType::Normal);  // Nf6
        board2.makeMove(move);

        REQUIRE(board2.enPassantSquare() == SQUARE_NONE);
    }
}

TEST_CASE("Board castling execution", "[board][move]") {
    Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    SECTION("White kingside castling") {
        Move move = Move::makeCastling(4, 6);  // e1-g1
        REQUIRE(board.makeMove(move));

        // King should be on g1
        REQUIRE(board.at(6).type() == PieceType::King);
        REQUIRE(board.at(6).color() == Color::White);
        REQUIRE(board.at(4).isEmpty());

        // Rook should be on f1
        REQUIRE(board.at(5).type() == PieceType::Rook);
        REQUIRE(board.at(5).color() == Color::White);
        REQUIRE(board.at(7).isEmpty());
    }

    SECTION("White queenside castling") {
        Move move = Move::makeCastling(4, 2);  // e1-c1
        REQUIRE(board.makeMove(move));

        // King should be on c1
        REQUIRE(board.at(2).type() == PieceType::King);
        REQUIRE(board.at(2).color() == Color::White);

        // Rook should be on d1
        REQUIRE(board.at(3).type() == PieceType::Rook);
        REQUIRE(board.at(3).color() == Color::White);
        REQUIRE(board.at(0).isEmpty());
    }
}

TEST_CASE("Board en passant execution", "[board][move]") {
    Board board("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");

    Move move = Move::makeEnPassant(36, 43);  // e5xd6
    REQUIRE(board.makeMove(move));

    // White pawn should be on d6
    REQUIRE(board.at(43).type() == PieceType::Pawn);
    REQUIRE(board.at(43).color() == Color::White);
    REQUIRE(board.at(36).isEmpty());

    // Black pawn on d5 should be captured
    REQUIRE(board.at(35).isEmpty());
}

TEST_CASE("Board promotion execution", "[board][move]") {
    Board board("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");

    SECTION("Promote to queen") {
        Move move = Move::makePromotion(48, 56, PieceType::Queen);
        REQUIRE(board.makeMove(move));

        REQUIRE(board.at(56).type() == PieceType::Queen);
        REQUIRE(board.at(56).color() == Color::White);
        REQUIRE(board.at(48).isEmpty());
    }

    SECTION("Promote to knight") {
        Move move = Move::makePromotion(48, 56, PieceType::Knight);
        REQUIRE(board.makeMove(move));

        REQUIRE(board.at(56).type() == PieceType::Knight);
        REQUIRE(board.at(56).color() == Color::White);
    }
}

TEST_CASE("Board FEN round-trip after moves", "[board][move]") {
    Board board;

    // Make a few moves
    board.makeMove(Move(12, 28, MoveType::Normal));  // e2-e4
    board.makeMove(Move(52, 36, MoveType::Normal));  // e7-e5
    board.makeMove(Move(6, 21, MoveType::Normal));   // Nf3

    // Serialize to FEN and create a new board
    std::string fen = board.toFen();
    Board board2(fen);

    // Both boards should be identical
    REQUIRE(board2.sideToMove() == board.sideToMove());
    REQUIRE(board2.castlingRights() == board.castlingRights());
    REQUIRE(board2.enPassantSquare() == board.enPassantSquare());
    REQUIRE(board2.halfmoveClock() == board.halfmoveClock());
    REQUIRE(board2.fullmoveNumber() == board.fullmoveNumber());
}

TEST_CASE("Board game state queries", "[board][move]") {
    SECTION("isInCheck") {
        Board board("4k3/8/8/8/8/8/8/K3R3 b - - 0 1");
        REQUIRE(board.isInCheck());
    }

    SECTION("isCheckmate") {
        Board board("R5k1/5ppp/8/8/8/8/8/7K b - - 0 1");
        REQUIRE(board.isCheckmate());
    }

    SECTION("isStalemate") {
        Board board("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        REQUIRE(board.isStalemate());
    }

    SECTION("isDraw - 50 move rule") {
        Board board("4k3/8/8/8/8/8/8/4K3 w - - 100 1");
        REQUIRE(board.isDraw());
    }
}

TEST_CASE("Board getLegalMoves", "[board][move]") {
    Board board;

    auto moves = board.getLegalMoves();
    REQUIRE(moves.size() == 20);  // Starting position has 20 legal moves
}

TEST_CASE("Board isMoveLegal", "[board][move]") {
    Board board;

    Move legalMove(12, 28, MoveType::Normal);  // e2-e4
    REQUIRE(board.isMoveLegal(legalMove));

    Move illegalMove(12, 36, MoveType::Normal);  // e2-e5 (pawn can't move 3 squares)
    REQUIRE_FALSE(board.isMoveLegal(illegalMove));
}
