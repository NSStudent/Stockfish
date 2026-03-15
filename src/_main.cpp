#include "_main.h"

#include <iostream>
#include <memory>

#include "bitboard.h"
#include "misc.h"
#include "position.h"
#include "tune.h"
#include "uci.h"

using namespace Stockfish;

int _main(int argc, char* argv[]) {
  std::cout << engine_info() << std::endl;

  Bitboards::init();
  Position::init();

  auto uci = std::make_unique<UCIEngine>(argc, argv);

  Tune::init(uci->engine_options());
  uci->loop();

  return 0;
}
