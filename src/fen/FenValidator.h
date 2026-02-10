#ifndef CCHESS_FENVALIDATOR_H
#define CCHESS_FENVALIDATOR_H

#include "../core/Position.h"

#include <string>

namespace cchess {

class FenValidator {
public:
    // Validate a position for logical consistency
    static bool validate(const Position& position, std::string* error = nullptr);

    // Individual validation checks
    static bool validateKings(const Position& position, std::string* error = nullptr);
    static bool validatePawns(const Position& position, std::string* error = nullptr);
    static bool validateEnPassant(const Position& position, std::string* error = nullptr);

private:
    // Count pieces of a specific type and color
    static int countPieces(const Position& position, PieceType type, Color color);
};

}  // namespace cchess

#endif  // CCHESS_FENVALIDATOR_H
