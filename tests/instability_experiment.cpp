#include "binary_field.hpp"
#include "heat_stencil.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: instability_experiment OUTPUT_FILE [CSV_FILE]\n";
        return 1;
    }

    try {
        constexpr int N = 32;
        constexpr int steps = 200;
        constexpr int sample_interval = 20;
        constexpr double r = 0.2625;
        Grid current(N);
        Grid next(N);
        const double angle = std::acos(-1.0) * (N - 1) / N;
        for (int j = 1; j < N; ++j) {
            for (int i = 1; i < N; ++i) {
                current(i, j) = 1e-6 * std::sin(angle * i) * std::sin(angle * j);
            }
        }

        std::ofstream csv;
        if (argc == 3) {
            csv.exceptions(std::ios::failbit | std::ios::badbit);
            csv.open(argv[2]);
            csv << "iteration,max_abs_temperature\n"
                << std::setprecision(std::numeric_limits<double>::max_digits10);
        }

        for (int step = 0; step <= steps; ++step) {
            if (csv.is_open() && step % sample_interval == 0) {
                double peak = 0.0;
                for (std::size_t i = 0; i < current.size(); ++i) {
                    peak = std::max(peak, std::abs(current.data()[i]));
                }
                csv << step << ',' << peak << '\n';
            }
            if (step == steps) {
                break;
            }
            detail::update_interior(current, next, r);
            std::swap(current, next);
        }
        if (csv.is_open()) {
            csv.close();
        }
        write_binary_field(current, argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
