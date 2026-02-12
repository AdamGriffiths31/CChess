#ifndef CCHESS_SEARCH_CONFIG_H
#define CCHESS_SEARCH_CONFIG_H

#include <chrono>

namespace cchess {

struct SearchConfig {
    std::chrono::milliseconds searchTime{1000};
    int maxDepth{64};
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_CONFIG_H
