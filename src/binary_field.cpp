#include "binary_field.hpp"

#include "grid.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

void write_binary_field(const Grid& grid, const std::string& filename) {
    static_assert(CHAR_BIT == 8 && sizeof(double) == 8 &&
                  std::numeric_limits<double>::is_iec559,
                  "Field output requires IEEE 754 binary64 doubles and 8-bit bytes");

    std::ofstream output(filename, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Cannot open output file '" + filename + "'");
    }

    constexpr std::size_t values_per_block = 4096;
    std::array<unsigned char, values_per_block * sizeof(double)> buffer;
    for (std::size_t offset = 0; offset < grid.size();) {
        const auto count = std::min(values_per_block, grid.size() - offset);
        for (std::size_t i = 0; i < count; ++i) {
            std::uint64_t bits;
            std::memcpy(&bits, grid.data() + offset + i, sizeof(bits));
            for (std::size_t byte = 0; byte < sizeof(bits); ++byte) {
                buffer[i * sizeof(bits) + byte] = static_cast<unsigned char>(bits >> (8 * byte));
            }
        }
        output.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<std::streamsize>(count * sizeof(double)));
        if (!output) {
            throw std::runtime_error("Cannot write complete field to '" + filename + "'");
        }
        offset += count;
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Cannot finish writing output file '" + filename + "'");
    }
}
