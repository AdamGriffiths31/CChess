#include "FenParser.h"

#include "../core/Square.h"
#include "../core/Zobrist.h"
#include "../utils/Error.h"
#include "../utils/StringUtils.h"

#include <sstream>

namespace cchess {

Position FenParser::parse(const std::string& fen) {
    auto fields = split(fen, ' ');

    if (fields.size() != 6) {
        throw FenParseError("FEN must have exactly 6 space-separated fields, got " +
                            std::to_string(fields.size()));
    }

    Position position;

    try {
        parsePiecePlacement(fields[0], position);
        position.setSideToMove(parseActiveColor(fields[1]));
        position.setCastlingRights(parseCastlingRights(fields[2]));
        position.setEnPassantSquare(parseEnPassantSquare(fields[3]));
        position.setHalfmoveClock(parseHalfmoveClock(fields[4]));
        position.setFullmoveNumber(parseFullmoveNumber(fields[5]));
    } catch (const FenParseError&) {
        throw;
    } catch (const std::exception& e) {
        throw FenParseError(std::string("Unexpected error: ") + e.what());
    }

    position.computeHash();
    return position;
}

std::string FenParser::serialize(const Position& position) {
    std::ostringstream oss;

    oss << serializePiecePlacement(position) << " " << serializeActiveColor(position.sideToMove())
        << " " << serializeCastlingRights(position.castlingRights()) << " "
        << serializeEnPassantSquare(position.enPassantSquare()) << " " << position.halfmoveClock()
        << " " << position.fullmoveNumber();

    return oss.str();
}

void FenParser::parsePiecePlacement(const std::string& field, Position& position) {
    auto ranks = split(field, '/');

    if (ranks.size() != 8) {
        throw FenParseError("Piece placement must have exactly 8 ranks, got " +
                            std::to_string(ranks.size()));
    }

    // Parse from rank 8 down to rank 1
    for (int rank = 7; rank >= 0; --rank) {
        const std::string& rankStr = ranks[static_cast<std::size_t>(7 - rank)];
        int file = 0;

        for (char c : rankStr) {
            if (c >= '1' && c <= '8') {
                // Empty squares
                int emptyCount = c - '0';
                file += emptyCount;
            } else {
                // Piece
                if (file >= 8) {
                    throw FenParseError("Rank " + std::to_string(rank + 1) +
                                        " has too many squares");
                }

                Piece piece = Piece::fromFenChar(c);
                if (piece.isEmpty()) {
                    throw FenParseError("Invalid piece character: " + std::string(1, c));
                }

                Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(rank));
                position.setPiece(sq, piece);
                ++file;
            }
        }

        if (file != 8) {
            throw FenParseError("Rank " + std::to_string(rank + 1) + " has " +
                                std::to_string(file) + " squares, expected 8");
        }
    }
}

Color FenParser::parseActiveColor(const std::string& field) {
    if (field == "w")
        return Color::White;
    if (field == "b")
        return Color::Black;
    throw FenParseError("Active color must be 'w' or 'b', got '" + field + "'");
}

CastlingRights FenParser::parseCastlingRights(const std::string& field) {
    if (field == "-") {
        return NO_CASTLING;
    }

    CastlingRights rights = NO_CASTLING;

    for (char c : field) {
        switch (c) {
            case 'K':
                rights |= WHITE_KINGSIDE;
                break;
            case 'Q':
                rights |= WHITE_QUEENSIDE;
                break;
            case 'k':
                rights |= BLACK_KINGSIDE;
                break;
            case 'q':
                rights |= BLACK_QUEENSIDE;
                break;
            default:
                throw FenParseError("Invalid castling rights character: " + std::string(1, c));
        }
    }

    return rights;
}

Square FenParser::parseEnPassantSquare(const std::string& field) {
    if (field == "-") {
        return SQUARE_NONE;
    }

    auto sq = stringToSquare(field);
    if (!sq) {
        throw FenParseError("Invalid en passant square: " + field);
    }

    // Validate en passant rank (must be rank 3 or rank 6)
    Rank rank = getRank(*sq);
    if (rank != RANK_3 && rank != RANK_6) {
        throw FenParseError("En passant square must be on rank 3 or 6, got " + field);
    }

    return *sq;
}

int FenParser::parseHalfmoveClock(const std::string& field) {
    if (!isInteger(field)) {
        throw FenParseError("Halfmove clock must be an integer, got '" + field + "'");
    }

    int clock = toInteger(field);
    if (clock < 0) {
        throw FenParseError("Halfmove clock must be non-negative, got " + std::to_string(clock));
    }

    return clock;
}

int FenParser::parseFullmoveNumber(const std::string& field) {
    if (!isInteger(field)) {
        throw FenParseError("Fullmove number must be an integer, got '" + field + "'");
    }

    int number = toInteger(field);
    if (number < 1) {
        throw FenParseError("Fullmove number must be at least 1, got " + std::to_string(number));
    }

    return number;
}

std::string FenParser::serializePiecePlacement(const Position& position) {
    std::ostringstream oss;

    // Serialize from rank 8 down to rank 1
    for (int rank = 7; rank >= 0; --rank) {
        if (rank < 7)
            oss << '/';

        int emptyCount = 0;
        for (int file = 0; file < 8; ++file) {
            Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(rank));
            const Piece& piece = position.pieceAt(sq);

            if (piece.isEmpty()) {
                ++emptyCount;
            } else {
                if (emptyCount > 0) {
                    oss << emptyCount;
                    emptyCount = 0;
                }
                oss << piece.toFenChar();
            }
        }

        if (emptyCount > 0) {
            oss << emptyCount;
        }
    }

    return oss.str();
}

std::string FenParser::serializeActiveColor(Color color) {
    return (color == Color::White) ? "w" : "b";
}

std::string FenParser::serializeCastlingRights(CastlingRights rights) {
    if (rights == NO_CASTLING) {
        return "-";
    }

    std::string result;
    if (rights & WHITE_KINGSIDE)
        result += 'K';
    if (rights & WHITE_QUEENSIDE)
        result += 'Q';
    if (rights & BLACK_KINGSIDE)
        result += 'k';
    if (rights & BLACK_QUEENSIDE)
        result += 'q';

    return result;
}

std::string FenParser::serializeEnPassantSquare(Square sq) {
    if (sq == SQUARE_NONE) {
        return "-";
    }
    return squareToString(sq);
}

}  // namespace cchess
