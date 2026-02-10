#ifndef CCHESS_FENPARSER_H
#define CCHESS_FENPARSER_H

#include "../core/Position.h"

#include <string>

namespace cchess {

class FenParser {
public:
    // Parse FEN string into Position
    static Position parse(const std::string& fen);

    // Serialize Position to FEN string
    static std::string serialize(const Position& position);

private:
    // Parse individual FEN fields
    static void parsePiecePlacement(const std::string& field, Position& position);
    static Color parseActiveColor(const std::string& field);
    static CastlingRights parseCastlingRights(const std::string& field);
    static Square parseEnPassantSquare(const std::string& field);
    static int parseHalfmoveClock(const std::string& field);
    static int parseFullmoveNumber(const std::string& field);

    // Serialize individual FEN fields
    static std::string serializePiecePlacement(const Position& position);
    static std::string serializeActiveColor(Color color);
    static std::string serializeCastlingRights(CastlingRights rights);
    static std::string serializeEnPassantSquare(Square sq);
};

}  // namespace cchess

#endif  // CCHESS_FENPARSER_H
