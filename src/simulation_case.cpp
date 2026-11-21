#include "simulation_case.hpp"

#include "grid.hpp"

#include <cmath>

void initialize_grid(Grid& grid, SimulationCase simulation_case) {
    const int N = grid.intervals();
    grid.fill(0.0);

    switch (simulation_case) {
    case SimulationCase::FourHotEdges:
        for (int j = 1; j < N; ++j) {
            grid(0, j) = 1.0;
            grid(N, j) = 1.0;
        }
        [[fallthrough]];
    case SimulationCase::HotTopAndBottom:
        for (int i = 0; i <= N; ++i) {
            grid(i, 0) = 1.0;
            grid(i, N) = 1.0;
        }
        break;
    case SimulationCase::Verification: {
        const double pi = std::acos(-1.0);
        for (int j = 1; j < N; ++j) {
            const double y = static_cast<double>(j) / N;
            const double sin_y = std::sin(pi * y);
            for (int i = 1; i < N; ++i) {
                const double x = static_cast<double>(i) / N;
                grid(i, j) = std::sin(pi * x) * sin_y;
            }
        }
        break;
    }
    }
}
