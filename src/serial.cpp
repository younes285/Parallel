#include "serial.hpp"
#include "heat_stencil.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace {
bool is_stable(double r) {
    return r > 0.0 && r <= 0.25;
}
}

void heat_step(const Grid& u_old, Grid& u_new, double r) {
    if (!is_stable(r)) {
        throw std::invalid_argument("heat_step requires 0 < r <= 0.25");
    }
    if (&u_old == &u_new) {
        throw std::invalid_argument("heat_step requires distinct input and output grids");
    }
    const int N = u_old.intervals();
    if (u_new.intervals() != N) {
        throw std::invalid_argument("heat_step requires matching grid dimensions");
    }

    detail::update_interior(u_old, u_new, r);
}

SerialResult run_serial(int N, SimulationCase simulation_case, double r, int steps) {
    if (N <= 0) {
        throw std::invalid_argument("run_serial requires a positive interval count");
    }
    if (steps < 0) {
        throw std::invalid_argument("run_serial requires a non-negative step count");
    }
    if (!is_stable(r)) {
        throw std::invalid_argument("run_serial requires 0 < r <= 0.25");
    }
    switch (simulation_case) {
    case SimulationCase::FourHotEdges:
    case SimulationCase::HotTopAndBottom:
    case SimulationCase::Verification:
        break;
    default:
        throw std::invalid_argument("run_serial requires a valid simulation case");
    }

    Grid first(N);
    Grid second(N);
    initialize_grid(first, simulation_case);
    initialize_grid(second, simulation_case);

    const double h = first.spacing();
    const double dt = r * (h * h);
    const double final_time = steps * dt;
    Grid* u_old = &first;
    Grid* u_new = &second;

    const auto start = std::chrono::steady_clock::now();
    for (int step = 0; step < steps; ++step) {
        heat_step(*u_old, *u_new, r);
        std::swap(u_old, u_new);
    }
    const auto end = std::chrono::steady_clock::now();

    const std::chrono::duration<double> elapsed = end - start;
    return {std::move(*u_old), dt, final_time, elapsed.count()};
}
