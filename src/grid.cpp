#include "grid.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
std::size_t node_count(int N) {
    if (N <= 0) {
        throw std::invalid_argument("Grid interval count must be positive");
    }

    const auto side = static_cast<std::size_t>(N) + 1;
    if (side > std::vector<double>().max_size() / side) {
        throw std::length_error("Grid contains too many nodes");
    }
    return side * side;
}
}

Grid::Grid(int N) : N_(N), temperatures_(node_count(N), 0.0) {}

std::size_t Grid::index(int i, int j) const {
    if (i < 0 || i > N_ || j < 0 || j > N_) {
        throw std::out_of_range("Grid coordinates must be between 0 and N");
    }

    const auto side = static_cast<std::size_t>(N_) + 1;
    return static_cast<std::size_t>(j) * side + static_cast<std::size_t>(i);
}

double& Grid::operator()(int i, int j) {
    return temperatures_[index(i, j)];
}

const double& Grid::operator()(int i, int j) const {
    return temperatures_[index(i, j)];
}

void Grid::fill(double temperature) {
    std::fill(temperatures_.begin(), temperatures_.end(), temperature);
}
