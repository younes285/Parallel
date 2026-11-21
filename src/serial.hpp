#pragma once

#include "grid.hpp"
#include "simulation_case.hpp"

// Both grids must be distinct and have the same dimensions.
// Initialize the fixed boundaries in both grids before calling.
// Only the interior of u_new is written.
void heat_step(const Grid& u_old, Grid& u_new, double r);

struct SerialResult {
    Grid grid;
    double dt;
    double final_time;
    double computation_seconds;
};

SerialResult run_serial(int N, SimulationCase simulation_case, double r, int steps);
