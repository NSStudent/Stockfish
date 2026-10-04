#include <cstddef>
#include <cstdint>

namespace Stockfish {
int fischer_syzygy_validate(const std::uint8_t*, std::size_t, bool);
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    (void) Stockfish::fischer_syzygy_validate(data, size, false);
    return 0;
}

#ifdef FISCHER_MUTATION_RUNNER
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

extern "C" void __sanitizer_set_death_callback(void (*)());

namespace {
std::uint64_t iteration;
std::vector<std::uint8_t> current;
const char* failureFile = "/private/tmp/fischer-stockfish-syzygy-failure.bin";

void save_failure() {
    std::ofstream stream(failureFile, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(current.data()), current.size());
    std::cerr << "Failing mutation " << iteration << " (seed 0x19), saved to " << failureFile << '\n';
}
}

int main(int argc, char* argv[]) {
    if (argc < 4)
    {
        std::cerr << "Usage: runner KQvK.rtbw KQvK.rtbz seconds [failure-file]\n";
        return 2;
    }
    if (argc > 4)
        failureFile = argv[4];
    __sanitizer_set_death_callback(save_failure);

    std::vector<std::vector<std::uint8_t>> seeds;
    for (int file = 1; file <= 2; ++file)
    {
        std::ifstream stream(argv[file], std::ios::binary);
        if (!stream)
            return 2;
        seeds.emplace_back(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>{});
        if (Stockfish::fischer_syzygy_validate(seeds.back().data(), seeds.back().size(), true) != 1)
            return 3;
    }

    std::uint64_t state = 0x19;
    auto random = [&]() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    };
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(std::strtol(argv[3], nullptr, 10));
    std::uint64_t rejected = 0, payload = 0, accepted = 0;
    do {
        ++iteration;
        current = seeds[random() % seeds.size()];
        const auto mode = random() % 5;
        if (mode == 0)
            current.resize(random() % current.size());
        else if (mode == 1)
            current.resize(current.size() + 64 * (1 + random() % 8), std::uint8_t(random()));
        const auto changes = 1 + random() % 8;
        for (unsigned i = 0; !current.empty() && i < changes; ++i)
        {
            const auto offset = random() % current.size();
            current[offset] = std::uint8_t(random());
        }
        const int result = Stockfish::fischer_syzygy_validate(current.data(), current.size(), false);
        rejected += result == 0;
        accepted += result == 1;
        payload += result == 2;
    } while (std::chrono::steady_clock::now() < deadline);
    std::cout << "seed=0x19 cases=" << iteration << " structure-rejected=" << rejected
              << " payload-rejected=" << payload << " decoded=" << accepted << '\n';
}
#endif
