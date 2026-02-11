#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace cchess {

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string trim(const std::string& str) {
    auto start =
        std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });

    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
                   return std::isspace(c);
               }).base();

    return (start < end) ? std::string(start, end) : std::string();
}

bool isInteger(const std::string& str) {
    if (str.empty())
        return false;

    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.length() == 1)
            return false;
        start = 1;
    }

    return std::all_of(str.begin() + static_cast<std::string::difference_type>(start), str.end(),
                       [](unsigned char c) { return std::isdigit(c); });
}

int toInteger(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

}  // namespace cchess
