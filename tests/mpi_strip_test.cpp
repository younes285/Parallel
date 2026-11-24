#include "mpi_strip.hpp"
#include "serial.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

void check_strip(const std::vector<int>& counts, int rank) {
    int intervals = 1;
    int first = 1;
    for (std::size_t i = 0; i < counts.size(); ++i) {
        intervals += counts[i];
        if (i < static_cast<std::size_t>(rank)) {
            first += counts[i];
        }
    }
    const int ranks = static_cast<int>(counts.size());
    MpiStrip strip(intervals, MPI_COMM_WORLD);
    if (strip.rank() != rank || strip.ranks() != ranks ||
        strip.first_global_row() != first || strip.owned_rows() != counts[rank] ||
        strip.lower_neighbor() != (rank == 0 ? MPI_PROC_NULL : rank - 1) ||
        strip.upper_neighbor() != (rank == ranks - 1 ? MPI_PROC_NULL : rank + 1) ||
        strip.columns() != static_cast<std::size_t>(intervals + 1) ||
        strip.size() != (counts[rank] + 2) * strip.columns() ||
        !std::all_of(strip.data(), strip.data() + strip.size(), [](double t) { return t == 0.0; })) {
        throw std::runtime_error("Unexpected partition or local storage");
    }

    const auto columns = strip.columns();
    const int owned = counts[rank];
    const auto temperature = [columns](int global_row, std::size_t column) {
        return static_cast<double>(global_row * columns + column);
    };
    double* const data = strip.data();
    std::fill_n(data, columns, -1.0);
    std::fill_n(data + (owned + 1) * columns, columns, -2.0);
    for (int row = 1; row <= owned; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            data[row * columns + column] = temperature(first + row - 1, column);
        }
    }

    strip.exchange_ghost_rows();

    for (std::size_t column = 0; column < columns; ++column) {
        const double bottom = rank == 0 ? -1.0 : temperature(first - 1, column);
        const double top = rank == ranks - 1 ? -2.0 : temperature(first + owned, column);
        if (data[column] != bottom || data[(owned + 1) * columns + column] != top) {
            throw std::runtime_error("Unexpected ghost row or modified physical boundary");
        }
        for (int row = 1; row <= owned; ++row) {
            if (data[row * columns + column] != temperature(first + row - 1, column)) {
                throw std::runtime_error("Ghost exchange modified an owned row");
            }
        }
    }
}

void check_local_step(int N, SimulationCase simulation_case) {
    constexpr double r = 0.2;
    Grid initial(N);
    initialize_grid(initial, simulation_case);
    // Distinct interior values expose row-order and ghost-selection errors.
    for (int j = 1; j < N; ++j) {
        for (int i = 1; i < N; ++i) {
            initial(i, j) += 0.01 * (i + 3 * j);
        }
    }
    Grid expected = initial;
    heat_step(initial, expected, r);

    MpiStrip current(N, MPI_COMM_WORLD);
    MpiStrip next(N, MPI_COMM_WORLD);
    const auto columns = current.columns();
    const int owned = current.owned_rows();
    const int first = current.first_global_row();
    std::fill_n(current.data(), current.size(), -1.0);
    std::fill_n(next.data(), next.size(), -2.0);
    for (int local = 0; local <= owned + 1; ++local) {
        const int global = first + local - 1;
        const bool neighbor_ghost = (local == 0 && global != 0) ||
                                    (local == owned + 1 && global != N);
        if (!neighbor_ghost) {
            std::copy_n(initial.data() + global * columns, columns,
                        current.data() + local * columns);
            std::copy_n(current.data() + local * columns, columns,
                        next.data() + local * columns);
        }
    }
    current.exchange_ghost_rows();
    const std::vector<double> snapshot(current.data(), current.data() + current.size());
    current.heat_step(next, r);

    for (int local = 0; local <= owned + 1; ++local) {
        const int global = first + local - 1;
        const bool neighbor_ghost = (local == 0 && global != 0) ||
                                    (local == owned + 1 && global != N);
        for (int i = 0; i <= N; ++i) {
            const double value = neighbor_ghost ? -2.0 : expected(i, global);
            if (next.data()[local * columns + i] != value) {
                throw std::runtime_error("Local MPI step differs from serial or overwrote a ghost");
            }
        }
    }
    if (!std::equal(snapshot.begin(), snapshot.end(), current.data())) {
        throw std::runtime_error("Local MPI step modified its current buffer");
    }
}

void check_simulation(int N, SimulationCase simulation_case, int steps) {
    constexpr double r = 0.2;
    const auto expected = run_serial(N, simulation_case, r, steps);
    const auto result = run_mpi_strip(N, simulation_case, r, steps, MPI_COMM_WORLD);
    auto strip = result.strip;
    if (result.dt != expected.dt || result.final_time != expected.final_time ||
        !(result.t_total >= 0.0 && result.t_comp >= 0.0 && result.t_comm >= 0.0) ||
        result.t_comp + result.t_comm > result.t_total + 1e-12 ||
        (steps == 0 && (result.t_comp != 0.0 || result.t_comm != 0.0))) {
        throw std::runtime_error("Unexpected MPI simulation timing");
    }
    const int begin = strip.lower_neighbor() == MPI_PROC_NULL ? 0 : 1;
    const int end = strip.owned_rows() + (strip.upper_neighbor() == MPI_PROC_NULL ? 1 : 0);
    for (int row = begin; row <= end; ++row) {
        const int global = strip.first_global_row() + (row - 1);
        for (int i = 0; i <= N; ++i) {
            if (strip.data()[row * strip.columns() + i] != expected.grid(i, global)) {
                throw std::runtime_error("MPI simulation differs from serial");
            }
        }
    }
    // Poison ghosts to check that the gather uses only owned rows.
    std::fill_n(strip.data(), strip.columns(), -1.0);
    std::fill_n(strip.data() + (strip.owned_rows() + 1) * strip.columns(),
                strip.columns(), -2.0);
    const auto gathered = strip.gather_global_field(simulation_case);
    if (gathered.has_value() != (strip.rank() == 0)) {
        throw std::runtime_error("Only rank zero should receive the global field");
    }
    if (gathered && (gathered->intervals() != N || gathered->size() != expected.grid.size() ||
                     !std::equal(expected.grid.data(), expected.grid.data() + expected.grid.size(),
                                 gathered->data()))) {
        throw std::runtime_error("Gathered field differs from serial");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank;
    int ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    int failed = 0;
    try {
        const std::vector<std::vector<int>> expected{{7}, {4, 3}, {3, 2, 2}, {2, 2, 2, 1}};
        check_strip(expected.at(ranks - 1), rank);
        check_strip(std::vector<int>(ranks, 1), rank);
        for (auto simulation_case : {SimulationCase::FourHotEdges,
                                     SimulationCase::HotTopAndBottom,
                                     SimulationCase::Verification}) {
            check_local_step(8, simulation_case);
            check_local_step(ranks + 1, simulation_case);
            check_simulation(8, simulation_case, 0);
            check_simulation(8, simulation_case, 11);
            check_simulation(2 * ranks + 1, simulation_case, 12);
            check_simulation(ranks + 1, simulation_case, 11);
        }
        try {
            const MpiStrip invalid(ranks, MPI_COMM_WORLD);
            throw std::runtime_error("Accepted a partition with too few rows");
        } catch (const std::invalid_argument&) {
        }
    } catch (const std::exception& error) {
        std::cerr << "Rank " << rank << ": " << error.what() << '\n';
        failed = 1;
    }
    MPI_Finalize();
    return failed;
}
