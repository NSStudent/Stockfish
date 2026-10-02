// Library entry point; the host owns the UCI transport and catches embedded errors.
#include "_main.h"

#include <iostream>
#include <memory>
#include <utility>

#include "attacks.h"
#include "misc.h"
#include "position.h"
#include "tune.h"
#include "uci.h"

int _main(int argc, char* argv[]) {
    using namespace Stockfish;
    std::cout << engine_info() << std::endl;
    Attacks::init();
    Position::init();
    auto cli = CommandLine(argc, argv);
    auto uci = std::make_unique<UCIEngine>(std::move(cli));
    Tune::init(uci->engine_options());
    uci->loop();
    return 0;
}
