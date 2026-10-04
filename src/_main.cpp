// Library entry point; the host owns the UCI transport and catches embedded errors.
#include "_main.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

#include "attacks.h"
#include "misc.h"
#include "position.h"
#include "tune.h"
#include "uci.h"

int _main(int argc, char* argv[], std::function<void(std::string_view)> onFailure) {
    using namespace Stockfish;
    // The logger must detach while this session's stream buffers are still alive.
    struct LoggerSession {
        LoggerSession() { start_logger(""); }
        ~LoggerSession() { start_logger(""); }
    } loggerSession;
    std::cout << engine_info() << std::endl;
    static std::once_flag initialized;
    std::call_once(initialized, [] {
        Attacks::init();
        Position::init();
    });
    auto cli = CommandLine(argc, argv);
    auto uci = std::make_unique<UCIEngine>(std::move(cli));
    uci->set_on_failure(std::move(onFailure));
    Tune::init(uci->engine_options());
    uci->loop();
    return 0;
}
