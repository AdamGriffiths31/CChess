#include "Square.h"

namespace cchess {

std::string squareToString(Square sq) {
    if (!squareIsValid(sq))
        return "-";

    std::string result;
    result += fileToChar(getFile(sq));
    result += rankToChar(getRank(sq));
    return result;
}

std::optional<Square> stringToSquare(const std::string& str) {
    if (str.length() != 2)
        return std::nullopt;

    auto file = charToFile(str[0]);
    auto rank = charToRank(str[1]);

    if (!file || !rank)
        return std::nullopt;

    return makeSquare(*file, *rank);
}

char fileToChar(File f) {
    return static_cast<char>('a' + f);
}

char rankToChar(Rank r) {
    return static_cast<char>('1' + r);
}

std::optional<File> charToFile(char c) {
    if (c >= 'a' && c <= 'h') {
        return static_cast<File>(c - 'a');
    }
    if (c >= 'A' && c <= 'H') {
        return static_cast<File>(c - 'A');
    }
    return std::nullopt;
}

std::optional<Rank> charToRank(char c) {
    if (c >= '1' && c <= '8') {
        return static_cast<Rank>(c - '1');
    }
    return std::nullopt;
}

}  // namespace cchess
