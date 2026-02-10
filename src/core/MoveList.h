#ifndef CCHESS_MOVELIST_H
#define CCHESS_MOVELIST_H

#include "Move.h"

#include <array>
#include <cstddef>

namespace cchess {

class MoveList {
public:
    using value_type = Move;
    MoveList() : size_(0) {}

    void push_back(const Move& move) { moves_[size_++] = move; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    const Move& operator[](size_t i) const { return moves_[i]; }
    Move& operator[](size_t i) { return moves_[i]; }

    const Move* begin() const { return moves_.data(); }
    const Move* end() const { return moves_.data() + size_; }
    Move* begin() { return moves_.data(); }
    Move* end() { return moves_.data() + size_; }

private:
    std::array<Move, 256> moves_;
    size_t size_;
};

}  // namespace cchess

#endif  // CCHESS_MOVELIST_H
