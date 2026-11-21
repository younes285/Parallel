#include "grid.hpp"
#include "serial.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Operation>
void require_invalid_argument(Operation operation, const char* message) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

void check_uniform_field() {
    Grid old_grid(4);
    Grid new_grid(4);
    old_grid.fill(8.0);
    new_grid.fill(8.0);
    new_grid(2, 2) = -1.0;

    heat_step(old_grid, new_grid, 0.25);
    require(std::all_of(new_grid.data(), new_grid.data() + new_grid.size(),
                        [](double value) { return value == 8.0; }),
            "A uniform field must stay uniform at the stability limit");
}

void check_manual_step() {
    Grid old_grid(4);
    Grid new_grid(4);
    old_grid(2, 2) = 8.0;
    new_grid.fill(-1.0);
    const Grid before = old_grid;

    heat_step(old_grid, new_grid, 0.125);

    // The center keeps half its heat. Each of its four neighbors receives one eighth.
    const std::array<double, 25> expected{
        -1.0, -1.0, -1.0, -1.0, -1.0,
        -1.0,  0.0,  1.0,  0.0, -1.0,
        -1.0,  1.0,  4.0,  1.0, -1.0,
        -1.0,  0.0,  1.0,  0.0, -1.0,
        -1.0, -1.0, -1.0, -1.0, -1.0
    };
    require(std::equal(expected.begin(), expected.end(), new_grid.data()),
            "The output must match the hand calculation and preserve the destination edges");
    require(std::equal(before.data(), before.data() + before.size(), old_grid.data()),
            "The source grid must stay unchanged");
}

void check_cold_center() {
    Grid old_grid(2);
    Grid new_grid(2);
    old_grid.fill(1.0);
    new_grid.fill(1.0);
    old_grid(1, 1) = 0.0;

    heat_step(old_grid, new_grid, 0.125);
    require(new_grid(1, 1) == 0.5, "A cold center surrounded by hot nodes must warm up");
}

void check_invalid_inputs() {
    Grid old_grid(4);
    Grid new_grid(4);
    for (double r : {0.0, -0.125, 0.251, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity(),
                     -std::numeric_limits<double>::infinity()}) {
        require_invalid_argument([&] { heat_step(old_grid, new_grid, r); },
                                 "An invalid r must be rejected");
    }
    require_invalid_argument([&] { heat_step(old_grid, old_grid, 0.125); },
                             "An in-place step must be rejected");
    Grid small_grid(2);
    require_invalid_argument([&] { heat_step(old_grid, small_grid, 0.125); },
                             "Mismatched grid dimensions must be rejected");
}
}

int main() {
    try {
        check_uniform_field();
        check_manual_step();
        check_cold_center();
        check_invalid_inputs();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
