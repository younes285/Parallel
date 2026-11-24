#pragma once

#include "simulation_case.hpp"

#include <string>

enum class Decomposition { Serial, Strips, Blocks };

struct Configuration {
    SimulationCase simulation_case;
    int N;
    double r;
    int steps;
    std::string output_filename;
    Decomposition decomposition;
};

Configuration parse_arguments(int argc, char* argv[]);
