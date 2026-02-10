#include "core/Move.h"
#include "core/Types.h"

#include <catch2/catch_test_macros.hpp>

using namespace cchess;

// Common squares for testing
static constexpr Square E2 = 12;  // e2
static constexpr Square E4 = 28;  // e4
static constexpr Square E7 = 52;  // e7
static constexpr Square E8 = 60;  // e8
static constexpr Square D5 = 35;  // d5
static constexpr Square E1 = 4;   // e1
static constexpr Square G1 = 6;   // g1

TEST_CASE("Move default constructor creates null move", "[move]") {
    Move move;
    REQUIRE(move.isNull());
    REQUIRE(move.from() == SQUARE_NONE);
    REQUIRE(move.to() == SQUARE_NONE);
}

TEST_CASE("Move normal construction", "[move]") {
    Move move(E2, E4, MoveType::Normal);
    REQUIRE_FALSE(move.isNull());
    REQUIRE(move.from() == E2);
    REQUIRE(move.to() == E4);
    REQUIRE(move.type() == MoveType::Normal);
    REQUIRE_FALSE(move.isCapture());
    REQUIRE_FALSE(move.isPromotion());
    REQUIRE_FALSE(move.isCastling());
    REQUIRE_FALSE(move.isEnPassant());
}

TEST_CASE("Move capture construction", "[move]") {
    Move move(E4, D5, MoveType::Capture);
    REQUIRE(move.from() == E4);
    REQUIRE(move.to() == D5);
    REQUIRE(move.type() == MoveType::Capture);
    REQUIRE(move.isCapture());
    REQUIRE_FALSE(move.isPromotion());
}

TEST_CASE("Move castling construction", "[move]") {
    Move move = Move::makeCastling(E1, G1);
    REQUIRE(move.from() == E1);
    REQUIRE(move.to() == G1);
    REQUIRE(move.isCastling());
    REQUIRE_FALSE(move.isCapture());
    REQUIRE_FALSE(move.isPromotion());
}

TEST_CASE("Move en passant construction", "[move]") {
    Move move = Move::makeEnPassant(E4, D5);
    REQUIRE(move.from() == E4);
    REQUIRE(move.to() == D5);
    REQUIRE(move.isEnPassant());
    REQUIRE(move.isCapture());  // En passant is a capture
    REQUIRE_FALSE(move.isPromotion());
}

TEST_CASE("Move promotion construction", "[move]") {
    Move move = Move::makePromotion(E7, E8, PieceType::Queen);
    REQUIRE(move.from() == E7);
    REQUIRE(move.to() == E8);
    REQUIRE(move.isPromotion());
    REQUIRE(move.promotion() == PieceType::Queen);
    REQUIRE_FALSE(move.isCapture());
}

TEST_CASE("Move promotion capture construction", "[move]") {
    Move move = Move::makePromotionCapture(E7, E8, PieceType::Rook);
    REQUIRE(move.from() == E7);
    REQUIRE(move.to() == E8);
    REQUIRE(move.isPromotion());
    REQUIRE(move.isCapture());
    REQUIRE(move.promotion() == PieceType::Rook);
}

TEST_CASE("Move toAlgebraic for normal move", "[move]") {
    Move move(E2, E4, MoveType::Normal);
    REQUIRE(move.toAlgebraic() == "e2e4");
}

TEST_CASE("Move toAlgebraic for promotion", "[move]") {
    SECTION("Queen promotion") {
        Move move = Move::makePromotion(E7, E8, PieceType::Queen);
        REQUIRE(move.toAlgebraic() == "e7e8q");
    }

    SECTION("Rook promotion") {
        Move move = Move::makePromotion(E7, E8, PieceType::Rook);
        REQUIRE(move.toAlgebraic() == "e7e8r");
    }

    SECTION("Bishop promotion") {
        Move move = Move::makePromotion(E7, E8, PieceType::Bishop);
        REQUIRE(move.toAlgebraic() == "e7e8b");
    }

    SECTION("Knight promotion") {
        Move move = Move::makePromotion(E7, E8, PieceType::Knight);
        REQUIRE(move.toAlgebraic() == "e7e8n");
    }
}

TEST_CASE("Move toAlgebraic for null move", "[move]") {
    Move move;
    REQUIRE(move.toAlgebraic() == "0000");
}

TEST_CASE("Move fromAlgebraic for normal move", "[move]") {
    auto move = Move::fromAlgebraic("e2e4");
    REQUIRE(move.has_value());
    REQUIRE(move->from() == E2);
    REQUIRE(move->to() == E4);
    REQUIRE(move->type() == MoveType::Normal);
}

TEST_CASE("Move fromAlgebraic for promotion", "[move]") {
    SECTION("Queen promotion") {
        auto move = Move::fromAlgebraic("e7e8q");
        REQUIRE(move.has_value());
        REQUIRE(move->from() == E7);
        REQUIRE(move->to() == E8);
        REQUIRE(move->type() == MoveType::Promotion);
        REQUIRE(move->promotion() == PieceType::Queen);
    }

    SECTION("Rook promotion") {
        auto move = Move::fromAlgebraic("e7e8r");
        REQUIRE(move.has_value());
        REQUIRE(move->promotion() == PieceType::Rook);
    }

    SECTION("Bishop promotion") {
        auto move = Move::fromAlgebraic("e7e8b");
        REQUIRE(move.has_value());
        REQUIRE(move->promotion() == PieceType::Bishop);
    }

    SECTION("Knight promotion") {
        auto move = Move::fromAlgebraic("e7e8n");
        REQUIRE(move.has_value());
        REQUIRE(move->promotion() == PieceType::Knight);
    }
}

TEST_CASE("Move fromAlgebraic with invalid input", "[move]") {
    SECTION("Too short") {
        REQUIRE_FALSE(Move::fromAlgebraic("e2e").has_value());
    }

    SECTION("Too long") {
        REQUIRE_FALSE(Move::fromAlgebraic("e2e4q5").has_value());
    }

    SECTION("Invalid squares") {
        REQUIRE_FALSE(Move::fromAlgebraic("z9a1").has_value());
    }

    SECTION("Invalid promotion piece") {
        REQUIRE_FALSE(Move::fromAlgebraic("e7e8x").has_value());
    }

    SECTION("Empty string") {
        REQUIRE_FALSE(Move::fromAlgebraic("").has_value());
    }
}

TEST_CASE("Move equality operator", "[move]") {
    Move move1(E2, E4, MoveType::Normal);
    Move move2(E2, E4, MoveType::Normal);
    Move move3(E2, E4, MoveType::Capture);
    Move move4(E7, E8, MoveType::Normal);

    REQUIRE(move1 == move2);
    REQUIRE(move1 != move3);  // Different type
    REQUIRE(move1 != move4);  // Different squares
}

TEST_CASE("Move promotion equality", "[move]") {
    Move queenPromo1 = Move::makePromotion(E7, E8, PieceType::Queen);
    Move queenPromo2 = Move::makePromotion(E7, E8, PieceType::Queen);
    Move rookPromo = Move::makePromotion(E7, E8, PieceType::Rook);

    REQUIRE(queenPromo1 == queenPromo2);
    REQUIRE(queenPromo1 != rookPromo);  // Different promotion piece
}

TEST_CASE("Move round-trip conversion", "[move]") {
    Move original(E2, E4, MoveType::Normal);
    std::string algebraic = original.toAlgebraic();
    auto parsed = Move::fromAlgebraic(algebraic);

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->from() == original.from());
    REQUIRE(parsed->to() == original.to());
}

TEST_CASE("Move round-trip conversion for promotion", "[move]") {
    Move original = Move::makePromotion(E7, E8, PieceType::Knight);
    std::string algebraic = original.toAlgebraic();
    auto parsed = Move::fromAlgebraic(algebraic);

    REQUIRE(parsed.has_value());
    REQUIRE(parsed->from() == original.from());
    REQUIRE(parsed->to() == original.to());
    REQUIRE(parsed->promotion() == original.promotion());
}
