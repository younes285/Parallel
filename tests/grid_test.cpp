#include "grid.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Operation>
void require_throws(Operation operation, const char* message) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}
}

int main() {
    try {
        Grid grid(4);
        const Grid& view = grid;
        require(view.intervals() == 4 && view.spacing() == 0.25,
                "Incorrect grid dimensions or spacing");
        require(view.size() == 25, "Grid must contain 25 nodes");
        require(std::all_of(view.data(), view.data() + view.size(),
                            [](double value) { return value == 0.0; }),
                "Every temperature must start at zero");

        grid(2, 3) = 7.5;
        require(view(2, 3) == 7.5, "Temperature write did not persist");
        require(view.data()[17] == 7.5, "The x-index must vary fastest");
        grid.data()[24] = 3.0;
        require(view(4, 4) == 3.0, "The final node must be accessible");

        grid.fill(-2.5);
        require(std::all_of(view.data(), view.data() + view.size(),
                            [](double value) { return value == -2.5; }),
                "Fill must update every temperature");

        require_throws<std::invalid_argument>([] { Grid invalid(0); },
                                              "Zero intervals must be rejected");
        require_throws<std::invalid_argument>([] { Grid invalid(-1); },
                                              "Negative intervals must be rejected");
        require_throws<std::out_of_range>([&] { grid(5, 0) = 1.0; },
                                         "An out-of-range x-index must be rejected");
        require_throws<std::out_of_range>([&] { (void)view(0, -1); },
                                         "A negative y-index must be rejected");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
