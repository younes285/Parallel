#pragma once

class Grid;

enum class SimulationCase {
    FourHotEdges,
    HotTopAndBottom,
    Verification
};

void initialize_grid(Grid& grid, SimulationCase simulation_case);
