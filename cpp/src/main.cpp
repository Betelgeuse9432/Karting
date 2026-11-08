#include "karting/game.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        karting::Game game;
        return game.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
