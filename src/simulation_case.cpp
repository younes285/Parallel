#include "simulation_case.hpp"

#include "grid.hpp"

void initialize_grid(Grid& grid, SimulationCase simulation_case) {
    const int N = grid.intervals();
    const auto width = static_cast<std::size_t>(N) + 1;
    for (int j = 0; j <= N; ++j) {
        initialize_temperature_row(grid.data() + j * width, N, j, simulation_case);
    }
}
