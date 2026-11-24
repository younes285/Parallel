#include "mpi_block.hpp"
#include "heat_stencil.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
void partition_axis(int cells, int ranks, int coordinate, int& first, int& count) {
    const int base = cells / ranks;
    const int remainder = cells % ranks;
    first = 1 + coordinate * base + std::min(coordinate, remainder);
    count = base + (coordinate < remainder ? 1 : 0);
}
}

MpiBlock::MpiBlock(int intervals, MPI_Comm parent) : intervals_(intervals) {
    int ranks;
    MPI_Comm_size(parent, &ranks);
    MPI_Dims_create(ranks, 2, dimensions_.data());
    if (intervals <= 1 || intervals - 1 < std::max(dimensions_[0], dimensions_[1])) {
        throw std::invalid_argument("MPI blocks require at least one interior cell per axis per rank");
    }

    const int periods[] = {0, 0};
    MPI_Cart_create(parent, 2, dimensions_.data(), periods, 0, &cart_);
    MPI_Comm_rank(cart_, &rank_);
    MPI_Cart_coords(cart_, rank_, 2, coordinates_.data());
    MPI_Cart_shift(cart_, 0, 1, &bottom_, &top_);
    MPI_Cart_shift(cart_, 1, 1, &left_, &right_);
    partition_axis(intervals - 1, dimensions_[0], coordinates_[0], first_row_, rows_);
    partition_axis(intervals - 1, dimensions_[1], coordinates_[1], first_column_, columns_);

    try {
        if (local_rows() > temperatures_.max_size() / local_columns()) {
            throw std::length_error("MPI block contains too many nodes");
        }
        temperatures_.resize(local_rows() * local_columns());
    } catch (...) {
        MPI_Comm_free(&cart_);
        throw;
    }
}

MpiBlock::~MpiBlock() {
    MPI_Comm_free(&cart_);
}

void MpiBlock::exchange_halos() {
    const auto width = local_columns();
    if (width > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("MPI block row width exceeds the MPI datatype stride limit");
    }

    MPI_Datatype column;
    MPI_Type_vector(rows_, 1, static_cast<int>(width), MPI_DOUBLE, &column);
    MPI_Type_commit(&column);

    const auto cell = [this, width](std::size_t row, std::size_t col) {
        return data() + row * width + col;
    };
    const auto top_halo = local_rows() - 1;
    const auto right_halo = width - 1;

    constexpr int downward = 0;
    constexpr int upward = 1;
    constexpr int leftward = 2;
    constexpr int rightward = 3;
    MPI_Sendrecv(cell(1, 1), columns_, MPI_DOUBLE, bottom_, downward,
                 cell(top_halo, 1), columns_, MPI_DOUBLE, top_, downward,
                 cart_, MPI_STATUS_IGNORE);
    MPI_Sendrecv(cell(rows_, 1), columns_, MPI_DOUBLE, top_, upward,
                 cell(0, 1), columns_, MPI_DOUBLE, bottom_, upward,
                 cart_, MPI_STATUS_IGNORE);
    MPI_Sendrecv(cell(1, 1), 1, column, left_, leftward,
                 cell(1, right_halo), 1, column, right_, leftward,
                 cart_, MPI_STATUS_IGNORE);
    MPI_Sendrecv(cell(1, columns_), 1, column, right_, rightward,
                 cell(1, 0), 1, column, left_, rightward,
                 cart_, MPI_STATUS_IGNORE);

    MPI_Type_free(&column);
}

void MpiBlock::heat_step(MpiBlock& next, double r) const {
    if (!(r > 0.0 && r <= 0.25)) {
        throw std::invalid_argument("MPI block heat step requires 0 < r <= 0.25");
    }
    if (this == &next || rows_ != next.rows_ || columns_ != next.columns_ ||
        first_row_ != next.first_row_ || first_column_ != next.first_column_) {
        throw std::invalid_argument("MPI block heat step requires distinct blocks with matching bounds");
    }

    const auto width = local_columns();
    const double center_weight = 1.0 - 4.0 * r;
    const double* current = data();
    double* output = next.data();
    for (int row = 1; row <= rows_; ++row) {
        for (int column = 1; column <= columns_; ++column) {
            const auto cell = row * width + column;
            output[cell] = detail::heat_stencil(
                current[cell], current[cell - 1], current[cell + 1],
                current[cell - width], current[cell + width], center_weight, r);
        }
    }
}

std::optional<Grid> MpiBlock::gather_global_block_field(SimulationCase simulation_case) const {
    const int N = intervals_;
    const int interior = N - 1;
    if (interior > std::numeric_limits<int>::max() / interior) {
        throw std::length_error("MPI block gather exceeds the MPI count or displacement limit");
    }

    std::vector<double> packed(rows_ * columns_);
    for (int row = 0; row < rows_; ++row) {
        std::copy_n(data() + (row + 1) * local_columns() + 1, columns_,
                    packed.data() + row * columns_);
    }

    struct Bounds {
        int row;
        int column;
        int rows;
        int columns;
    };
    std::vector<Bounds> bounds;
    std::vector<int> counts;
    std::vector<int> offsets;
    std::vector<double> received;
    std::optional<Grid> field;
    const auto width = static_cast<std::size_t>(N) + 1;
    if (rank_ == 0) {
        const int ranks = dimensions_[0] * dimensions_[1];
        bounds.resize(ranks);
        counts.resize(ranks);
        offsets.resize(ranks);
        int offset = 0;
        for (int rank = 0; rank < ranks; ++rank) {
            int coords[2];
            MPI_Cart_coords(cart_, rank, 2, coords);
            auto& block = bounds[rank];
            partition_axis(interior, dimensions_[0], coords[0], block.row, block.rows);
            partition_axis(interior, dimensions_[1], coords[1], block.column, block.columns);
            counts[rank] = block.rows * block.columns;
            offsets[rank] = offset;
            offset += counts[rank];
        }
        received.resize(offset);
        field.emplace(N);
        initialize_temperature_row(field->data(), N, 0, simulation_case);
        initialize_temperature_row(field->data() + N * width, N, N, simulation_case);
        const double edge = simulation_case == SimulationCase::FourHotEdges ? 1.0 : 0.0;
        for (int row = 1; row < N; ++row) {
            field->data()[row * width] = edge;
            field->data()[row * width + N] = edge;
        }
    }

    MPI_Gatherv(packed.data(), static_cast<int>(packed.size()), MPI_DOUBLE,
                received.data(), counts.data(), offsets.data(), MPI_DOUBLE, 0, cart_);

    for (std::size_t rank = 0; rank < bounds.size(); ++rank) {
        const auto& block = bounds[rank];
        for (int row = 0; row < block.rows; ++row) {
            std::copy_n(received.data() + offsets[rank] + row * block.columns, block.columns,
                        field->data() + (block.row + row) * width + block.column);
        }
    }
    return field;
}

MpiBlockResult run_mpi_block(int N, SimulationCase simulation_case, double r, int steps,
                             MPI_Comm parent) {
    if (steps < 0 || !(r > 0.0 && r <= 0.25)) {
        throw std::invalid_argument("MPI block simulation requires non-negative steps and 0 < r <= 0.25");
    }
    switch (simulation_case) {
    case SimulationCase::FourHotEdges:
    case SimulationCase::HotTopAndBottom:
    case SimulationCase::Verification:
        break;
    default:
        throw std::invalid_argument("MPI block simulation requires a valid simulation case");
    }

    auto current = std::make_unique<MpiBlock>(N, parent);
    auto next = std::make_unique<MpiBlock>(N, parent);
    const auto width = current->local_columns();
    const auto height = current->local_rows();
    for (std::size_t row = 0; row < height; ++row) {
        const int j = current->first_global_row() - 1 + static_cast<int>(row);
        initialize_temperature_segment(current->data() + row * width, N, j,
                                       current->first_global_column() - 1, width, simulation_case);
    }
    std::copy_n(current->data(), current->size(), next->data());

    const double h = 1.0 / N;
    const double dt = r * (h * h);
    double t_comm = 0.0;
    double t_comp = 0.0;
    const double start = MPI_Wtime();
    for (int step = 0; step < steps; ++step) {
        const double exchange_start = MPI_Wtime();
        current->exchange_halos();
        const double compute_start = MPI_Wtime();
        current->heat_step(*next, r);
        const double compute_end = MPI_Wtime();
        t_comm += compute_start - exchange_start;
        t_comp += compute_end - compute_start;
        current.swap(next);
    }
    const double t_total = MPI_Wtime() - start;
    return {std::move(current), dt, steps * dt, t_total, t_comp, t_comm};
}
