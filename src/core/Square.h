#ifndef CCHESS_SQUARE_H
#define CCHESS_SQUARE_H

#include "Types.h"

#include <optional>
#include <string>

namespace cchess {

// Convert square to algebraic notation (e.g., "e4")
std::string squareToString(Square sq);

// Parse algebraic notation to square (e.g., "e4" -> 28)
std::optional<Square> stringToSquare(const std::string& str);

// Get file character ('a'-'h')
char fileToChar(File f);

// Get rank character ('1'-'8')
char rankToChar(Rank r);

// Parse file character to file number
std::optional<File> charToFile(char c);

// Parse rank character to rank number
std::optional<Rank> charToRank(char c);

}  // namespace cchess

#endif  // CCHESS_SQUARE_H
