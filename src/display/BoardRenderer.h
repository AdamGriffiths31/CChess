#ifndef CCHESS_BOARDRENDERER_H
#define CCHESS_BOARDRENDERER_H

#include "../core/Board.h"

#include <string>

namespace cchess {

class BoardRenderer {
public:
    // Render board to string
    static std::string render(const Board& board);

    // Render position information
    static std::string renderPositionInfo(const Board& board);

private:
    // Render individual components
    static std::string renderRank(const Board& board, Rank rank);
    static std::string renderSquare(const Piece& piece);
    static std::string renderCoordinates();
};

}  // namespace cchess

#endif  // CCHESS_BOARDRENDERER_H
