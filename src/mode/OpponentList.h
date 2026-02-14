#ifndef CCHESS_OPPONENT_LIST_H
#define CCHESS_OPPONENT_LIST_H

#include <map>
#include <string>
#include <vector>

namespace cchess {

struct Opponent {
    std::string name;
    std::string enginePath;  // Absolute path to engine executable
    std::map<std::string, std::string> options;
};

// Load opponents from a JSON config file.
// Engine paths in the JSON are relative to the directory containing the JSON file.
std::vector<Opponent> loadOpponents(const std::string& jsonPath);

}  // namespace cchess

#endif  // CCHESS_OPPONENT_LIST_H
