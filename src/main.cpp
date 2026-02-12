#include "core/Board.h"
#include "display/BoardRenderer.h"
#include "mode/PlayerVsPlayer.h"
#include "mode/PerftRunner.h"
#include "mode/StsRunner.h"
#include "core/Square.h"
#include "utils/Error.h"
#include <iostream>
#include <exception>
#include <limits>

void showMenu() {
    std::cout << "\n========== CChess ==========\n";
    std::cout << "1. Player vs Player\n";
    std::cout << "2. Perft Test\n";
    std::cout << "3. STS Benchmark\n";
    std::cout << "4. Exit\n";
    std::cout << "===========================\n";
    std::cout << "Select option: ";
}

int getMenuChoice() {
    int choice;
    while (!(std::cin >> choice) || choice < 1 || choice > 4) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please enter 1-4: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

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
                    cchess::PerftRunner::run();
                    break;
                case 3:
                    cchess::StsRunner::run();
                    break;
                case 4:
                    std::cout << "Thanks for playing!\n";
                    return 0;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
