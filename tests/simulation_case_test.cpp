#include "grid.hpp"
#include "simulation_case.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require_temperature(const Grid& grid, int i, int j, double expected,
                         double tolerance, const char* case_name) {
    const double actual = grid(i, j);
    if (!(std::abs(actual - expected) <= tolerance)) {
        throw std::runtime_error(std::string(case_name) + ", N=" +
                                 std::to_string(grid.intervals()) +
                                 ", node (" + std::to_string(i) + ", " +
                                 std::to_string(j) + "): expected " +
                                 std::to_string(expected) + ", got " +
                                 std::to_string(actual));
    }
}

void check_cases(int N) {
    Grid grid(N);
    grid.fill(9.0);

    initialize_grid(grid, SimulationCase::FourHotEdges);
    for (int j = 0; j <= N; ++j) {
        for (int i = 0; i <= N; ++i) {
            const bool boundary = i == 0 || i == N || j == 0 || j == N;
            require_temperature(grid, i, j, boundary ? 1.0 : 0.0, 0.0, "Case 1");
        }
    }

    initialize_grid(grid, SimulationCase::HotTopAndBottom);
    for (int j = 0; j <= N; ++j) {
        for (int i = 0; i <= N; ++i) {
            require_temperature(grid, i, j, j == 0 || j == N ? 1.0 : 0.0, 0.0, "Case 2");
        }
    }

    initialize_grid(grid, SimulationCase::Verification);
    for (int k = 0; k <= N; ++k) {
        require_temperature(grid, k, 0, 0.0, 0.0, "Case V");
        require_temperature(grid, k, N, 0.0, 0.0, "Case V");
        require_temperature(grid, 0, k, 0.0, 0.0, "Case V");
        require_temperature(grid, N, k, 0.0, 0.0, "Case V");
    }
    require_temperature(grid, N / 2, N / 2, 1.0, 1e-14, "Case V");
    require_temperature(grid, N / 4, N / 4, 0.5, 1e-14, "Case V");

    initialize_grid(grid, SimulationCase::FourHotEdges);
    require_temperature(grid, N / 2, N / 2, 0.0, 0.0, "Case 1 after Case V");
}
}

int main() {
    try {
        for (int N : {4, 8, 16}) {
            check_cases(N);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
