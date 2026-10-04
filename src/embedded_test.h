#ifndef STOCKFISH_EMBEDDED_TEST_H_INCLUDED
#define STOCKFISH_EMBEDDED_TEST_H_INCLUDED

// Fault injection is compiled only into the native regression harness.
#ifdef FISCHER_TESTING
#include <atomic>

namespace Stockfish::EmbeddedTestHooks {
inline std::atomic<int> allocationCountdown{-1};
inline std::atomic<int> threadCountdown{-1};

inline bool consume_failure(std::atomic<int>& countdown) {
    int value = countdown.load();
    while (value >= 0)
        if (countdown.compare_exchange_weak(value, value - 1))
            return value == 0;
    return false;
}
}
#endif
#endif
