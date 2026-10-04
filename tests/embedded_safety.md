The embedded harnesses compile separately from SwiftPM and never load NNUE for
each Syzygy mutation. From the Stockfish checkout:

```sh
rtk proxy python3 tests/embedded_safety.py syzygy
rtk proxy python3 tests/embedded_safety.py mutate
rtk proxy python3 tests/embedded_safety.py nnue
rtk proxy python3 tests/embedded_safety.py native
rtk proxy /private/tmp/fischer-stockfish-audit/stockfish-nnue
rtk proxy /private/tmp/fischer-stockfish-audit/stockfish-native
rtk proxy /private/tmp/fischer-stockfish-audit/stockfish-syzygy /path/KQvK.rtbw /path/KQvK.rtbz
rtk proxy /private/tmp/fischer-stockfish-audit/stockfish-mutate /path/KQvK.rtbw /path/KQvK.rtbz 1800
```

Use the checksummed real fixtures from the host repository's
`Scripts/prepare-stockfish19-tablebases.py`. ASan/UBSan stops on the first
violation. The mutation runner uses seed `0x19`, reports its case count and saves
a failing input to `/private/tmp/fischer-stockfish-syzygy-failure.bin`. On toolchains
with libFuzzer, build `embedded_syzygy_fuzzer.cpp` without
`FISCHER_MUTATION_RUNNER` and link `-fsanitize=fuzzer,address,undefined`.

`FISCHER_FUZZING` exposes only a test parser for KQvK. It walks the production
structure validator and bounded decoder, sampling 64 indices per side or every
index for the original regression fixtures. The regression runner tries every
truncation and four mutations at every byte, then asserts rejection of invalid
magic, material flags, group ordering and piece descriptors. This tiny material
corpus exercises compressed WDL/DTZ data and mappings; it does not cover every
legal pawn or seven-piece encoding.

`FISCHER_TESTING` enables `EmbeddedTestHooks::threadCountdown` and
`allocationCountdown`: `0` fails the next search thread/aligned allocation,
`1` the second, and `-1` disables injection. Failures are consumed once. Test-only
`Engine::fischer_test_job`, `fischer_test_wait` and `fischer_test_stopped` exercise
worker exceptions and stop-before-join without running a chess search. The native
regression harness verifies restart with logging, partial thread/allocation
rollback, failure callback deduplication and destruction while a job is active.

Embedded errors unwind to the host's error boundary; worker errors stop their
pool and notify the host once. A failed session must be destroyed before a new
session starts. A timeout cannot make an unresponsive native worker safe to free.
Mapped tables and NNUE shared memory must remain unchanged while in use: external
modification/truncation can cause OS signals such as SIGBUS, which C++ exceptions
cannot recover. Global memory exhaustion and internal engine invariant failures
also remain outside this recovery guarantee.
