#include "mpi_strip.hpp"
#include "heat_stencil.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
struct StripRows {
    int first;
    int count;
};

StripRows partition_rows(int interior_rows, int ranks, int rank) {
    const int base = interior_rows / ranks;
    const int extra = interior_rows % ranks;
    return {1 + rank * base + std::min(rank, extra), base + (rank < extra ? 1 : 0)};
}
}

MpiStrip::MpiStrip(int intervals, MPI_Comm comm) : comm_(comm) {
    MPI_Comm_rank(comm, &rank_);
    MPI_Comm_size(comm, &ranks_);
    if (intervals <= 1 || intervals - 1 < ranks_) {
        throw std::invalid_argument("MPI strips require at least one interior row per rank");
    }

    const auto rows = partition_rows(intervals - 1, ranks_, rank_);
    owned_rows_ = rows.count;
    first_global_row_ = rows.first;
    lower_neighbor_ = rank_ == 0 ? MPI_PROC_NULL : rank_ - 1;
    upper_neighbor_ = rank_ == ranks_ - 1 ? MPI_PROC_NULL : rank_ + 1;

    columns_ = static_cast<std::size_t>(intervals) + 1;
    const auto local_rows = static_cast<std::size_t>(owned_rows_) + 2;
    if (local_rows > temperatures_.max_size() / columns_) {
        throw std::length_error("MPI strip contains too many nodes");
    }
    temperatures_.resize(local_rows * columns_);
}

std::optional<Grid> MpiStrip::gather_global_field(SimulationCase simulation_case) const {
    const int N = static_cast<int>(columns_ - 1);
    const auto max_rows = std::numeric_limits<int>::max() / columns_;
    const auto first_strip = partition_rows(N - 1, ranks_, 0);
    const auto last_strip = partition_rows(N - 1, ranks_, ranks_ - 1);
    if (static_cast<std::size_t>(first_strip.count) > max_rows ||
        static_cast<std::size_t>(last_strip.first) > max_rows) {
        throw std::length_error("MPI strip gather exceeds the MPI count or displacement limit");
    }

    std::optional<Grid> field;
    std::vector<int> counts;
    std::vector<int> displacements;
    if (rank_ == 0) {
        field.emplace(N);
        counts.resize(ranks_);
        displacements.resize(ranks_);
        for (int rank = 0; rank < ranks_; ++rank) {
            const auto rows = partition_rows(N - 1, ranks_, rank);
            counts[rank] = static_cast<int>(rows.count * columns_);
            displacements[rank] = static_cast<int>(rows.first * columns_);
        }
        initialize_temperature_row(field->data(), N, 0, simulation_case);
        initialize_temperature_row(field->data() + N * columns_, N, N, simulation_case);
    }

    MPI_Gatherv(data() + columns_, static_cast<int>(owned_rows_ * columns_), MPI_DOUBLE,
                field ? field->data() : nullptr, counts.data(), displacements.data(),
                MPI_DOUBLE, 0, comm_);

    if (field) {
        const double edge = simulation_case == SimulationCase::FourHotEdges ? 1.0 : 0.0;
        for (int j = 1; j < N; ++j) {
            (*field)(0, j) = edge;
            (*field)(N, j) = edge;
        }
    }
    return field;
}

void MpiStrip::exchange_ghost_rows() {
    if (columns_ > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("MPI strip row exceeds the MPI message count limit");
    }

    constexpr int upward_tag = 0;
    constexpr int downward_tag = 1;
    const int count = static_cast<int>(columns_);
    double* const bottom_ghost = data();
    double* const bottom_owned = bottom_ghost + columns_;
    double* const top_owned = bottom_ghost + owned_rows_ * columns_;
    double* const top_ghost = top_owned + columns_;

    // Rows are stored from bottom to top, so the last stored owned row goes upward.
    MPI_Sendrecv(top_owned, count, MPI_DOUBLE, upper_neighbor_, upward_tag,
                 bottom_ghost, count, MPI_DOUBLE, lower_neighbor_, upward_tag,
                 comm_, MPI_STATUS_IGNORE);
    MPI_Sendrecv(bottom_owned, count, MPI_DOUBLE, lower_neighbor_, downward_tag,
                 top_ghost, count, MPI_DOUBLE, upper_neighbor_, downward_tag,
                 comm_, MPI_STATUS_IGNORE);
}

void MpiStrip::heat_step(MpiStrip& next, double r) const {
    if (!(r > 0.0 && r <= 0.25)) {
        throw std::invalid_argument("MPI strip heat step requires 0 < r <= 0.25");
    }
    if (this == &next || comm_ != next.comm_ || columns_ != next.columns_) {
        throw std::invalid_argument("MPI strip heat step requires distinct strips with matching layouts");
    }

    const double center_weight = 1.0 - 4.0 * r;
    const double* current = data();
    double* output = next.data();
    for (int row = 1; row <= owned_rows_; ++row) {
        for (std::size_t column = 1; column < columns_ - 1; ++column) {
            const auto cell = row * columns_ + column;
            output[cell] = detail::heat_stencil(
                current[cell], current[cell - 1], current[cell + 1],
                current[cell - columns_], current[cell + columns_], center_weight, r);
        }
    }
}

MpiStripResult run_mpi_strip(int N, SimulationCase simulation_case, double r, int steps,
                             MPI_Comm comm) {
    if (steps < 0 || !(r > 0.0 && r <= 0.25)) {
        throw std::invalid_argument("MPI strip simulation requires non-negative steps and 0 < r <= 0.25");
    }
    switch (simulation_case) {
    case SimulationCase::FourHotEdges:
    case SimulationCase::HotTopAndBottom:
    case SimulationCase::Verification:
        break;
    default:
        throw std::invalid_argument("MPI strip simulation requires a valid simulation case");
    }

    MpiStrip first(N, comm);
    MpiStrip second(N, comm);
    const int begin = first.lower_neighbor() == MPI_PROC_NULL ? 0 : 1;
    const int end = first.owned_rows() + (first.upper_neighbor() == MPI_PROC_NULL ? 1 : 0);
    for (int row = begin; row <= end; ++row) {
        const int global_row = first.first_global_row() + (row - 1);
        const auto offset = row * first.columns();
        initialize_temperature_row(first.data() + offset, N, global_row, simulation_case);
        std::copy_n(first.data() + offset, first.columns(), second.data() + offset);
    }

    const double h = 1.0 / N;
    const double dt = r * (h * h);
    MpiStrip* current = &first;
    MpiStrip* next = &second;
    double t_comm = 0.0;
    double t_comp = 0.0;
    const double start = MPI_Wtime();
    for (int step = 0; step < steps; ++step) {
        const double exchange_start = MPI_Wtime();
        current->exchange_ghost_rows();
        const double compute_start = MPI_Wtime();
        current->heat_step(*next, r);
        const double compute_end = MPI_Wtime();
        t_comm += compute_start - exchange_start;
        t_comp += compute_end - compute_start;
        std::swap(current, next);
    }
    const double t_total = MPI_Wtime() - start;
    return {std::move(*current), dt, steps * dt, t_total, t_comp, t_comm};
}
