#pragma once

#include "simulation_case.hpp"

#include <string>

struct Configuration {
    SimulationCase simulation_case;
    int N;
    double r;
    int steps;
    std::string output_filename;
};

Configuration parse_arguments(int argc, char* argv[]);
