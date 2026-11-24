#pragma once

#include <algorithm>
#include <cmath>

class Grid;

enum class SimulationCase {
    FourHotEdges,
    HotTopAndBottom,
    Verification
};

void initialize_grid(Grid& grid, SimulationCase simulation_case);

// Fill a contiguous segment of global row j, beginning at column first.
inline void initialize_temperature_segment(double* row, int N, int j, int first,
                                           std::size_t count, SimulationCase simulation_case) {
    if (j == 0 || j == N) {
        const double boundary = (simulation_case == SimulationCase::FourHotEdges ||
                                 simulation_case == SimulationCase::HotTopAndBottom) ? 1.0 : 0.0;
        std::fill_n(row, count, boundary);
        return;
    }

    const double edge = simulation_case == SimulationCase::FourHotEdges ? 1.0 : 0.0;
    const bool verification = simulation_case == SimulationCase::Verification;
    const double pi = std::acos(-1.0);
    const double sin_y = verification ? std::sin(pi * (static_cast<double>(j) / N)) : 0.0;
    for (std::size_t offset = 0; offset < count; ++offset) {
        const int i = first + static_cast<int>(offset);
        if (i == 0 || i == N) {
            row[offset] = edge;
        } else if (verification) {
            const double x = static_cast<double>(i) / N;
            row[offset] = std::sin(pi * x) * sin_y;
        } else {
            row[offset] = 0.0;
        }
    }
}

inline void initialize_temperature_row(double* row, int N, int j, SimulationCase simulation_case) {
    initialize_temperature_segment(row, N, j, 0, static_cast<std::size_t>(N) + 1, simulation_case);
}
