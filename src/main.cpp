#include "core/Board.h"
#include "core/Square.h"
#include "core/Zobrist.h"
#include "display/BoardRenderer.h"
#include "mode/EngineMatch.h"
#include "mode/OpponentList.h"
#include "mode/PerftRunner.h"
#include "mode/PlayerVsPlayer.h"
#include "mode/StsRunner.h"
#include "uci/Uci.h"
#include "utils/Error.h"

#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <vector>

void showMenu() {
    std::cout << "\n========== CChess ==========\n";
    std::cout << "1. Player vs Player\n";
    std::cout << "2. Play vs Engine\n";
    std::cout << "3. Perft Test\n";
    std::cout << "4. STS Benchmark\n";
    std::cout << "5. Exit\n";
    std::cout << "===========================\n";
    std::cout << "Select option: ";
}

int getMenuChoice() {
    int choice;
    while (!(std::cin >> choice) || choice < 1 || choice > 5) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please enter 1-5: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void playVsEngine() {
    std::vector<cchess::Opponent> opponents;
    try {
        opponents = cchess::loadOpponents("engines/opponents.json");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load opponents: " << e.what() << "\n";
        return;
    }

    if (opponents.empty()) {
        std::cout << "No opponents configured in engines/opponents.json\n";
        return;
    }

    std::cout << "\n--- Select Opponent ---\n";
    for (size_t i = 0; i < opponents.size(); ++i) {
        std::cout << (i + 1) << ". " << opponents[i].name << "\n";
    }
    std::cout << (opponents.size() + 1) << ". Back\n";
    std::cout << "Choice: ";

    int pick;
    int maxPick = static_cast<int>(opponents.size()) + 1;
    while (!(std::cin >> pick) || pick < 1 || pick > maxPick) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please enter 1-" << maxPick << ": ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (pick == maxPick)
        return;

    cchess::EngineMatch match(opponents[static_cast<size_t>(pick - 1)]);
    match.play();
}

int main(int argc, char* argv[]) {
    cchess::zobrist::init();

    // UCI mode via command-line flag
    if (argc > 1 && std::strcmp(argv[1], "--uci") == 0) {
        cchess::Uci uci;
        uci.loop();
        return 0;
    }

    try {
        while (true) {
            showMenu();
            int choice = getMenuChoice();

            switch (choice) {
                case 1: {
                    cchess::PlayerVsPlayer game;
                    game.play();
                    break;
                }
                case 2:
                    playVsEngine();
                    break;
                case 3:
                    cchess::PerftRunner::run();
                    break;
                case 4:
                    cchess::StsRunner::run();
                    break;
                case 5:
                    std::cout << "Thanks for playing!\n";
                    return 0;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
