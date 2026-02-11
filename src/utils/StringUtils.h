#ifndef CCHESS_STRINGUTILS_H
#define CCHESS_STRINGUTILS_H

#include <string>
#include <vector>

namespace cchess {

// Split string by delimiter
std::vector<std::string> split(const std::string& str, char delimiter);

// Trim whitespace from both ends
std::string trim(const std::string& str);

// Check if string is a valid integer
bool isInteger(const std::string& str);

// Parse string to integer (returns 0 if invalid)
int toInteger(const std::string& str);

}  // namespace cchess

#endif  // CCHESS_STRINGUTILS_H
