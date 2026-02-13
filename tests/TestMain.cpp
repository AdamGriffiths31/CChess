#define CATCH_CONFIG_RUNNER
#include "core/Zobrist.h"

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    cchess::zobrist::init();
    return Catch::Session().run(argc, argv);
}
