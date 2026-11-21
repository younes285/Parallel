#include "grid.hpp"
#include "verification.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

void require_near(double actual, double expected, const char* message) {
    if (!(std::abs(actual - expected) <= 1e-14)) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        const double pi = std::acos(-1.0);
        require_near(exact_temperature(0.25, 0.25, 0.0), 0.5, "Incorrect spatial mode");
        require_near(exact_temperature(0.5, 0.5, std::log(2.0) / (2.0 * pi * pi)),
                     0.5, "Incorrect time decay");
        require_near(exact_temperature(1.0, 0.5, 0.0), 0.0, "Incorrect boundary");

        Grid grid(2);
        grid(1, 1) = 1.0;
        const auto zero = verification_errors(grid, 0.0);
        require_near(zero.infinity, 0.0, "Identical fields must have zero maximum error");
        require_near(zero.l2, 0.0, "Identical fields must have zero L2 error");

        grid(1, 1) += 3.0;
        grid(2, 0) = -4.0;
        const auto errors = verification_errors(grid, 0.0);
        require_near(errors.infinity, 4.0, "Incorrect maximum error");
        require_near(errors.l2, 2.5, "Incorrect weighted L2 error");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
