#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace Stockfish {
int fischer_syzygy_validate(const std::uint8_t*, std::size_t, bool);
}

int main(int argc, char* argv[]) {
    assert(argc == 3 && "Pass real KQvK.rtbw and KQvK.rtbz fixtures");
    for (int file = 1; file < argc; ++file)
    {
        std::ifstream stream(argv[file], std::ios::binary);
        assert(stream && "Fixture missing");
        std::vector<std::uint8_t> original((std::istreambuf_iterator<char>(stream)), {});
        assert(Stockfish::fischer_syzygy_validate(original.data(), original.size(), true) == 1);

        // Every truncation must remain bounded; a suffix containing only padding
        // can be absent while retaining valid data, so rejection is not required.
        for (std::size_t size = 0; size < original.size(); ++size)
            (void) Stockfish::fischer_syzygy_validate(original.data(), size, false);

        // Exercise malformed flags, counts, code lengths, child references,
        // sparse indices, block lengths and compressed payload in both formats.
        // These are bounded regression samples, independent of NNUE or threads.
        for (std::size_t offset = 0; offset < original.size(); ++offset)
            for (std::uint8_t value : {0, 0x10, 0x80, 0xFF})
            {
                auto changed = original;
                changed[offset] = value;
                (void) Stockfish::fischer_syzygy_validate(changed.data(), changed.size(), false);
            }

        auto bad = original;
        bad[0] ^= 0xFF;
        assert(Stockfish::fischer_syzygy_validate(bad.data(), bad.size(), true) == 0);
        bad = original;
        bad[4] ^= 2;  // A pawnless file cannot describe pawn groups.
        assert(Stockfish::fischer_syzygy_validate(bad.data(), bad.size(), true) == 0);
        bad = original;
        bad[5] = 0xFF;  // The order cannot name a nonexistent group.
        assert(Stockfish::fischer_syzygy_validate(bad.data(), bad.size(), true) == 0);
        bad = original;
        bad[6] = 0;  // A group cannot contain an empty square.
        assert(Stockfish::fischer_syzygy_validate(bad.data(), bad.size(), true) == 0);
        std::cout << "Validated parser/decoder and bounded mutations: " << argv[file] << '\n';
    }
}
