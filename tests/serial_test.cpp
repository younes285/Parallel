#include "serial.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_invalid(int N, SimulationCase simulation_case, double r, int steps) {
    try {
        run_serial(N, simulation_case, r, steps);
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Invalid simulation parameters must be rejected");
}

void check_small_grid() {
    constexpr auto simulation_case = SimulationCase::FourHotEdges;
    constexpr double r = 0.125;
    Grid initial(2);
    initialize_grid(initial, simulation_case);

    const auto zero = run_serial(2, simulation_case, r, 0);
    require(std::equal(initial.data(), initial.data() + initial.size(), zero.grid.data()),
            "Zero steps must return the initial grid");
    require(zero.final_time == 0.0, "Zero steps must give zero simulated time");

    Grid direct(2);
    initialize_grid(direct, simulation_case);
    heat_step(initial, direct, r);
    const auto one = run_serial(2, simulation_case, r, 1);
    require(std::equal(direct.data(), direct.data() + direct.size(), one.grid.data()),
            "One step must match a direct heat_step call");

    // The sole interior node follows u_next = u / 2 + 1 / 2.
    require(one.grid(1, 1) == 0.5, "The center must be 0.5 after one step");
    const auto two = run_serial(2, simulation_case, r, 2);
    require(two.grid(1, 1) == 0.75, "The center must be 0.75 after two steps");
    const auto three = run_serial(2, simulation_case, r, 3);
    require(three.grid(1, 1) == 0.875, "The center must be 0.875 after three steps");
    require(three.dt == 0.03125 && three.final_time == 0.09375,
            "Simulated time must use dt = r * h^2");
    require(zero.computation_seconds >= 0.0 && three.computation_seconds >= 0.0,
            "Elapsed computation time must be non-negative");
}

void check_boundaries() {
    constexpr int N = 4;
    for (auto simulation_case : {SimulationCase::FourHotEdges,
                                 SimulationCase::HotTopAndBottom,
                                 SimulationCase::Verification}) {
        const auto result = run_serial(N, simulation_case, 0.25, 5);
        const Grid& grid = result.grid;
        for (int j = 0; j <= N; ++j) {
            for (int i = 0; i <= N; ++i) {
                const double value = grid(i, j);
                if (simulation_case != SimulationCase::Verification) {
                    require(value >= 0.0 && value <= 1.0, "Temperature left [0, 1]");
                }
                const bool horizontal_edge = j == 0 || j == N;
                if (horizontal_edge || i == 0 || i == N) {
                    const bool hot = simulation_case == SimulationCase::FourHotEdges ||
                                     (simulation_case == SimulationCase::HotTopAndBottom &&
                                      horizontal_edge);
                    require(value == (hot ? 1.0 : 0.0), "A fixed boundary changed");
                }
            }
        }
    }
}

void check_invalid_inputs() {
    constexpr auto simulation_case = SimulationCase::FourHotEdges;
    require_invalid(0, simulation_case, 0.125, 1);
    require_invalid(4, simulation_case, 0.125, -1);
    require_invalid(4, static_cast<SimulationCase>(-1), 0.125, 1);
    for (double r : {0.0, -0.125, 0.251, std::numeric_limits<double>::quiet_NaN(),
                     std::numeric_limits<double>::infinity()}) {
        require_invalid(4, simulation_case, r, 0);
    }
}
}

int main() {
    try {
        check_small_grid();
        check_boundaries();
        check_invalid_inputs();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
