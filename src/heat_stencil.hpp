#pragma once

#include "grid.hpp"

namespace detail {
// Callers supply distinct, equally sized grids with initialized boundaries.
inline void update_interior(const Grid& u_old, Grid& u_new, double r) {
    const int N = u_old.intervals();
    const double center_weight = 1.0 - 4.0 * r;
    for (int j = 1; j < N; ++j) {
        for (int i = 1; i < N; ++i) {
            const double neighbors = u_old(i - 1, j) + u_old(i + 1, j)
                                   + u_old(i, j - 1) + u_old(i, j + 1);
            u_new(i, j) = center_weight * u_old(i, j) + r * neighbors;
        }
    }
}
}
