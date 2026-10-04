#include "../src/_main.h"
#include "../src/embedded_test.h"
#include "../src/engine.h"
#include "../src/memory.h"
#include "../src/misc.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace Stockfish;

namespace {
struct Faulty {
    static inline int live = 0, attempts = 0;
    Faulty() {
        if (++attempts == 3)
            throw std::runtime_error("constructor fault");
        ++live;
    }
    ~Faulty() { --live; }
};

template<typename Fn>
void expects_failure(Fn&& fn) {
    bool caught = false;
    try { fn(); } catch (const std::exception&) { caught = true; }
    assert(caught);
}

void set_option(Engine& engine, const std::string& command) {
    std::istringstream stream(command);
    engine.get_options().setoption(stream);
}
}

int main() {
    // Each stream buffer has a different lifetime. Logging must detach before
    // the old session dies and attach to the new session's buffers on restart.
    auto* inputBuffer = std::cin.rdbuf();
    auto* outputBuffer = std::cout.rdbuf();
    for (int session = 0; session < 3; ++session)
    {
        std::istringstream input("setoption name Debug Log File value /private/tmp/fischer-stockfish-native.log\nuci\nquit\n");
        std::ostringstream output;
        std::cin.rdbuf(input.rdbuf());
        std::cout.rdbuf(output.rdbuf());
        char name[] = "embedded-regression";
        char* argv[] = {name};
        assert(_main(1, argv) == 0);
        assert(std::cin.rdbuf() == input.rdbuf());
        assert(std::cout.rdbuf() == output.rdbuf());
        assert(output.str().find("uciok") != std::string::npos);
        std::cin.rdbuf(inputBuffer);
        std::cout.rdbuf(outputBuffer);
    }

    // Failed object construction must release raw aligned allocation and
    // destruct already constructed array elements in reverse order.
    expects_failure([] { auto array = make_unique_aligned<Faulty[]>(5); });
    assert(Faulty::live == 0);
    expects_failure([] { auto array = make_unique_aligned<int[]>(std::numeric_limits<usize>::max()); });

    // Creation failure during construction and after one of several workers
    // has started must join every worker that actually started.
    EmbeddedTestHooks::threadCountdown = 0;
    expects_failure([] { Engine engine; });
    {
        Engine engine;
        EmbeddedTestHooks::threadCountdown = 1;
        expects_failure([&] { set_option(engine, "name Threads value 3"); });
    }
    EmbeddedTestHooks::allocationCountdown = 0;
    expects_failure([] { Engine engine; });
    {
        Engine engine;
        EmbeddedTestHooks::allocationCountdown = 1;
        expects_failure([&] { set_option(engine, "name Threads value 3"); });
    }

    // Exceptions from jobs and even failure callbacks cannot cross a native
    // thread boundary. The first failure poisons the session and is reported once.
    {
        Engine engine;
        std::atomic<int> reports{0};
        engine.set_on_failure([&](std::string_view message) {
            assert(message == "worker fault");
            ++reports;
            throw std::runtime_error("callback fault");
        });
        engine.fischer_test_job(0, [] { throw std::runtime_error("worker fault"); });
        expects_failure([&] { engine.fischer_test_wait(0); });
        engine.fischer_test_job(0, [] { throw std::bad_alloc(); });
        expects_failure([&] { engine.fischer_test_wait(0); });
        assert(reports == 1);
    }

    // Destroying an active engine signals stop before waiting for its worker.
    {
        auto engine = std::make_unique<Engine>();
        auto* pointer = engine.get();
        std::atomic<bool> started{false}, ended{false};
        engine->fischer_test_job(0, [&] {
            started = true;
            while (!pointer->fischer_test_stopped())
                std::this_thread::yield();
            ended = true;
        });
        while (!started)
            std::this_thread::yield();
        const auto before = std::chrono::steady_clock::now();
        engine.reset();
        assert(ended);
        assert(std::chrono::steady_clock::now() - before < std::chrono::seconds(5));
    }
    std::cout << "Validated logger restart, allocation rollback, native thread failure and stop-before-join\n";
}
