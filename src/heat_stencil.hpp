#pragma once

#include "grid.hpp"

namespace detail {
inline double heat_stencil(double center, double left, double right, double bottom,
                           double top, double center_weight, double r) {
    const double neighbors = left + right + bottom + top;
    return center_weight * center + r * neighbors;
}

// Callers supply distinct, equally sized grids with initialized boundaries.
inline void update_interior(const Grid& u_old, Grid& u_new, double r) {
    const int N = u_old.intervals();
    const double center_weight = 1.0 - 4.0 * r;
    for (int j = 1; j < N; ++j) {
        for (int i = 1; i < N; ++i) {
            u_new(i, j) = heat_stencil(u_old(i, j), u_old(i - 1, j), u_old(i + 1, j),
                                       u_old(i, j - 1), u_old(i, j + 1), center_weight, r);
        }
    }
}
}
