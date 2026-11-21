#include "binary_field.hpp"
#include "heat_stencil.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <utility>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: instability_experiment OUTPUT_FILE\n";
        return 1;
    }

    try {
        constexpr int N = 32;
        constexpr int steps = 200;
        constexpr double r = 0.2625;
        Grid current(N);
        Grid next(N);
        const double angle = std::acos(-1.0) * (N - 1) / N;
        for (int j = 1; j < N; ++j) {
            for (int i = 1; i < N; ++i) {
                current(i, j) = 1e-6 * std::sin(angle * i) * std::sin(angle * j);
            }
        }

        for (int step = 0; step < steps; ++step) {
            detail::update_interior(current, next, r);
            std::swap(current, next);
        }
        write_binary_field(current, argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
