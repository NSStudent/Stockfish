#include "../src/nnue/nnue_common.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>

using namespace Stockfish;
using namespace Stockfish::Eval::NNUE;

template<typename T>
void roundtrip() {
    std::array<T, 7> values{std::numeric_limits<T>::min(), -2, -1, 0, 1, 2, std::numeric_limits<T>::max()};
    std::ostringstream output;
    write_leb_128(output, values.data(), values.size());
    std::istringstream input(output.str());
    std::array<T, 7> decoded{};
    read_leb_128(input, decoded.data(), decoded.size());
    assert(input && decoded == values);
}

int main() {
    roundtrip<std::int8_t>();
    roundtrip<std::int16_t>();
    roundtrip<std::int32_t>();
    for (const std::string& bytes : {std::string(8, char(0x80)), std::string("\x80\x80\x04", 3), std::string("\x80\x80", 2)})
    {
        std::ostringstream output;
        output.write(Leb128MagicString, Leb128MagicStringSize);
        write_little_endian<std::uint32_t>(output, std::uint32_t(bytes.size()));
        output.write(bytes.data(), bytes.size());
        std::istringstream input(output.str());
        std::int16_t decoded{};
        read_leb_128(input, &decoded, 1);
        assert(input.fail());
    }
    std::cout << "Validated NNUE signed LEB128 bounds and roundtrips\n";
}
