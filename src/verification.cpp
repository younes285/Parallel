#include "verification.hpp"

#include "grid.hpp"

#include <algorithm>
#include <cmath>

double exact_temperature(double x, double y, double time) {
    if (x == 0.0 || x == 1.0 || y == 0.0 || y == 1.0) {
        return 0.0;
    }
    const double pi = std::acos(-1.0);
    return std::sin(pi * x) * std::sin(pi * y) * std::exp(-2.0 * pi * pi * time);
}

VerificationErrors verification_errors(const Grid& grid, double time) {
    const int N = grid.intervals();
    double maximum = 0.0;
    double squared_sum = 0.0;
    for (int j = 0; j <= N; ++j) {
        for (int i = 0; i <= N; ++i) {
            const double exact = exact_temperature(static_cast<double>(i) / N,
                                                   static_cast<double>(j) / N, time);
            const double error = grid(i, j) - exact;
            maximum = std::max(maximum, std::abs(error));
            squared_sum += error * error;
        }
    }
    return {maximum, grid.spacing() * std::sqrt(squared_sum)};
}
