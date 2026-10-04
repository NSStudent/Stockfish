#include "../src/nnue/nnue_common.h"

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace Stockfish;
using namespace Stockfish::Eval::NNUE;

template<typename T>
bool parse(const std::string& bytes, std::size_t count) {
    std::array<T, 130> values;
    values.fill(T(67));
    std::istringstream input(bytes);
    read_leb_128(input, values.data() + 1, count);
    assert(values.front() == T(67));
    for (std::size_t i = count + 1; i < values.size(); ++i) assert(values[i] == T(67));
    if (!input) return false;
    std::ostringstream encoded;
    write_leb_128(encoded, values.data() + 1, count);
    std::istringstream roundtrip(encoded.str());
    std::array<T, 128> decoded{};
    read_leb_128(roundtrip, decoded.data(), count);
    assert(roundtrip);
    for (std::size_t i = 0; i < count; ++i) assert(decoded[i] == values[i + 1]);
    return true;
}

template<typename T>
std::string valid(std::mt19937& random, std::size_t count) {
    std::array<T, 128> values{};
    for (std::size_t i = 0; i < count; ++i) values[i] = T(random());
    if (count) values[0] = std::numeric_limits<T>::min();
    if (count > 1) values[1] = std::numeric_limits<T>::max();
    std::ostringstream output;
    write_leb_128(output, values.data(), count);
    return output.str();
}

void save(const char* path, std::size_t count, const std::string& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    write_little_endian<std::uint32_t>(output, std::uint32_t(count));
    output.write(bytes.data(), std::streamsize(bytes.size()));
    if (!output) throw std::runtime_error("Cannot preserve mutation input");
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--replay") {
        std::ifstream input(argv[2], std::ios::binary);
        const auto count = read_little_endian<std::uint32_t>(input);
        if (!input || count > 128) return 2;
        const std::string bytes{std::istreambuf_iterator<char>(input), {}};
        parse<std::int8_t>(bytes, count);
        parse<std::int16_t>(bytes, count);
        parse<std::int32_t>(bytes, count);
        return 0;
    }
    if (argc != 3) return 2;
    const auto seconds = std::stoul(argv[1]);
    if (seconds == 0 || seconds > 3600) return 2;
    std::mt19937 random(0x1919);
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    auto report = std::chrono::steady_clock::now();
    std::uint64_t cases = 0, accepted = 0;
    do {
        const std::size_t count = random() % 129;
        std::string bytes;
        switch (random() % 3) {
        case 0: bytes = valid<std::int8_t>(random, count); break;
        case 1: bytes = valid<std::int16_t>(random, count); break;
        default: bytes = valid<std::int32_t>(random, count); break;
        }
        switch (random() % 6) {
        case 0: break; // Keep known-valid cases in the corpus.
        case 1: bytes.resize(random() % (bytes.size() + 1)); break;
        case 2:
            for (unsigned i = 0, edits = 1 + random() % 8; i < edits; ++i)
                bytes[random() % bytes.size()] = char(random());
            break;
        case 3: {
            const auto offset = Leb128MagicStringSize;
            const std::uint32_t size = random() % 2 ? random() : UINT32_MAX;
            for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = char(size >> (8 * i));
            break;
        }
        case 4:
            bytes.append(1 + random() % 32, char(0x80));
            break;
        default:
            bytes.assign(random() % 1025, char(random()));
            break;
        }
        save(argv[2], count, bytes); // A sanitizer failure/deadline leaves its exact input.
        accepted += parse<std::int8_t>(bytes, count);
        accepted += parse<std::int16_t>(bytes, count);
        accepted += parse<std::int32_t>(bytes, count);
        ++cases;
        const auto now = std::chrono::steady_clock::now();
        if (now - report >= std::chrono::seconds(30)) {
            std::cout << "NNUE seed=0x1919 cases=" << cases << " accepted=" << accepted << std::endl;
            report = now;
        }
    } while (std::chrono::steady_clock::now() < until);
    std::cout << "NNUE complete seed=0x1919 cases=" << cases << " accepted=" << accepted << std::endl;
}
