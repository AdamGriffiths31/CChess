#ifndef CCHESS_SEARCH_CONFIG_H
#define CCHESS_SEARCH_CONFIG_H

#include <atomic>
#include <chrono>

namespace cchess {

struct SearchConfig {
    std::chrono::milliseconds searchTime{1000};
    int maxDepth{64};
    std::atomic<bool>* stopSignal = nullptr;  // external stop (for UCI "stop")
};

}  // namespace cchess

#endif  // CCHESS_SEARCH_CONFIG_H
