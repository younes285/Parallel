#include "mpi_block.hpp"
#include "serial.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void check_block(int N, int ranks) {
    MpiBlock block(N, MPI_COMM_WORLD);
    const auto dims = block.dimensions();
    const auto coords = block.coordinates();
    const int metadata[] = {coords[0], coords[1], block.first_global_row(),
                            block.first_global_column(), block.owned_rows(),
                            block.owned_columns()};
    std::vector<int> all(ranks * 6);
    MPI_Allgather(metadata, 6, MPI_INT, all.data(), 6, MPI_INT, block.communicator());

    require(dims[0] * dims[1] == ranks, "Incorrect process-grid size");
    require(ranks != 4 || dims == std::array<int, 2>{2, 2}, "Expected a 2x2 grid");
    std::vector<int> coverage((N - 1) * (N - 1));
    std::vector<int> rank_at(ranks, MPI_PROC_NULL);
    for (int rank = 0; rank < ranks; ++rank) {
        const int* entry = all.data() + rank * 6;
        for (int axis = 0; axis < 2; ++axis) {
            const int coordinate = entry[axis];
            const int quotient = (N - 1) / dims[axis];
            const int remainder = (N - 1) % dims[axis];
            require(coordinate >= 0 && coordinate < dims[axis], "Invalid coordinate");
            require(entry[axis + 4] == quotient + (coordinate < remainder),
                    "Incorrect partition size");
            require(entry[axis + 2] >= 1 && entry[axis + 2] + entry[axis + 4] <= N,
                    "Owned cells include a physical boundary");
        }
        const int index = entry[0] * dims[1] + entry[1];
        require(rank_at[index] == MPI_PROC_NULL, "Duplicate coordinates");
        rank_at[index] = rank;
        for (int j = entry[2]; j < entry[2] + entry[4]; ++j) {
            for (int i = entry[3]; i < entry[3] + entry[5]; ++i) {
                ++coverage[(j - 1) * (N - 1) + i - 1];
            }
        }
    }
    require(std::all_of(coverage.begin(), coverage.end(), [](int count) { return count == 1; }),
            "Interior coverage has gaps or overlaps");

    const auto neighbor = [&](int row, int column) {
        if (row < 0 || row >= dims[0] || column < 0 || column >= dims[1]) {
            return MPI_PROC_NULL;
        }
        return rank_at[row * dims[1] + column];
    };
    const int row = coords[0];
    const int column = coords[1];
    require(block.rank() == neighbor(row, column) &&
            block.bottom_neighbor() == neighbor(row - 1, column) &&
            block.top_neighbor() == neighbor(row + 1, column) &&
            block.left_neighbor() == neighbor(row, column - 1) &&
            block.right_neighbor() == neighbor(row, column + 1), "Incorrect Cartesian neighbors");
    require((block.first_global_row() == 1) == (block.bottom_neighbor() == MPI_PROC_NULL) &&
            (block.first_global_row() + block.owned_rows() == N) == (block.top_neighbor() == MPI_PROC_NULL) &&
            (block.first_global_column() == 1) == (block.left_neighbor() == MPI_PROC_NULL) &&
            (block.first_global_column() + block.owned_columns() == N) == (block.right_neighbor() == MPI_PROC_NULL),
            "Physical boundaries do not align with outer halo slots");
    require(block.local_rows() == static_cast<std::size_t>(block.owned_rows()) + 2 &&
            block.local_columns() == static_cast<std::size_t>(block.owned_columns()) + 2 &&
            block.size() == block.local_rows() * block.local_columns(), "Incorrect halo storage size");
}

void check_halos(int N) {
    MpiBlock block(N, MPI_COMM_WORLD);
    const auto height = block.local_rows();
    const auto width = block.local_columns();
    for (int case_index = 0; case_index < 3; ++case_index) {
        for (int pass = 0; pass < 2; ++pass) {
            const auto expected = [&](std::size_t row, std::size_t col, bool exchanged) {
                const int j = block.first_global_row() + (static_cast<int>(row) - 1);
                const int i = block.first_global_column() + (static_cast<int>(col) - 1);
                const bool halo_row = row == 0 || row == height - 1;
                const bool halo_col = col == 0 || col == width - 1;
                if (j == 0 || j == N) {
                    return case_index == 2 ? 0.0 : 1.0;
                }
                if (i == 0 || i == N) {
                    return case_index == 0 ? 1.0 : 0.0;
                }
                if ((halo_row && halo_col) || (!exchanged && (halo_row || halo_col))) {
                    return -1.0;
                }
                return static_cast<double>((pass + 1) * (N + 1) * (N + 1) + j * (N + 1) + i);
            };
            for (std::size_t row = 0; row < height; ++row) {
                for (std::size_t col = 0; col < width; ++col) {
                    block.data()[row * width + col] = expected(row, col, false);
                }
            }
            block.exchange_halos();
            for (std::size_t row = 0; row < height; ++row) {
                for (std::size_t col = 0; col < width; ++col) {
                    require(block.data()[row * width + col] == expected(row, col, true),
                            "Incorrect exchanged edge or modified boundary, corner, or owned cell");
                }
            }
        }
    }
}
}

void check_heat_step(int N, SimulationCase simulation_case) {
    constexpr double r = 0.2;
    Grid initial(N);
    initialize_grid(initial, simulation_case);
    for (int j = 1; j < N; ++j) {
        for (int i = 1; i < N; ++i) {
            initial(i, j) += 0.01 * (i + 3 * j);
        }
    }
    Grid expected = initial;
    heat_step(initial, expected, r);

    MpiBlock current(N, MPI_COMM_WORLD);
    MpiBlock next(N, MPI_COMM_WORLD);
    const auto width = current.local_columns();
    const auto height = current.local_rows();
    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t col = 0; col < width; ++col) {
            const int j = current.first_global_row() + (static_cast<int>(row) - 1);
            const int i = current.first_global_column() + (static_cast<int>(col) - 1);
            const bool physical = i == 0 || i == N || j == 0 || j == N;
            const bool owned = row > 0 && row + 1 < height && col > 0 && col + 1 < width;
            current.data()[row * width + col] = physical || owned ? initial(i, j) : -1.0;
            next.data()[row * width + col] = physical ? initial(i, j) : -2.0;
        }
    }
    current.exchange_halos();
    const std::vector<double> before(current.data(), current.data() + current.size());
    const std::vector<double> destination_before(next.data(), next.data() + next.size());
    current.heat_step(next, r);
    require(std::equal(before.begin(), before.end(), current.data()), "Current block was modified");

    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t col = 0; col < width; ++col) {
            const int j = current.first_global_row() + (static_cast<int>(row) - 1);
            const int i = current.first_global_column() + (static_cast<int>(col) - 1);
            const bool owned = row > 0 && row + 1 < height && col > 0 && col + 1 < width;
            const auto cell = row * width + col;
            require(next.data()[cell] == (owned ? expected(i, j) : destination_before[cell]),
                    "Block step differs from serial or overwrote a destination halo");
        }
    }
}

void check_simulation(int N, SimulationCase simulation_case, int steps, MPI_Comm parent) {
    constexpr double r = 0.2;
    const auto expected = run_serial(N, simulation_case, r, steps);
    const auto result = run_mpi_block(N, simulation_case, r, steps, parent);
    auto& block = *result.block;
    require(result.dt == expected.dt && result.final_time == expected.final_time &&
            result.t_comp >= 0 && result.t_comm >= 0 &&
            result.t_total + 1e-12 >= result.t_comp + result.t_comm &&
            (steps != 0 || (result.t_comp == 0 && result.t_comm == 0)),
            "Unexpected simulation timing");
    const auto width = block.local_columns();
    const auto height = block.local_rows();
    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t col = 0; col < width; ++col) {
            const int j = block.first_global_row() - 1 + static_cast<int>(row);
            const int i = block.first_global_column() - 1 + static_cast<int>(col);
            const bool physical = i == 0 || i == N || j == 0 || j == N;
            const bool owned = row > 0 && row + 1 < height && col > 0 && col + 1 < width;
            if (physical || owned) {
                require(block.data()[row * width + col] == expected.grid(i, j),
                        "Final MPI block differs from serial");
            }
            if (!owned) {
                block.data()[row * width + col] = -123.0;
            }
        }
    }
    const auto field = block.gather_global_block_field(simulation_case);
    require(field.has_value() == (block.rank() == 0), "Gather returned a field on the wrong rank");
    if (field) {
        require(field->intervals() == N && field->size() == expected.grid.size() &&
                std::equal(field->data(), field->data() + field->size(), expected.grid.data()),
                "Gathered field differs from serial");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int ranks;
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm reversed;
    MPI_Comm_split(MPI_COMM_WORLD, 0, ranks - 1 - world_rank, &reversed);
    try {
        check_block(13, ranks);
        check_block(8, ranks);
        check_halos(13);
        check_halos(8);
        check_halos(ranks == 1 ? 2 : ranks == 3 ? 4 : 3);
        for (auto simulation_case : {SimulationCase::FourHotEdges,
                                     SimulationCase::HotTopAndBottom,
                                     SimulationCase::Verification}) {
            check_heat_step(13, simulation_case);
            check_heat_step(8, simulation_case);
            check_heat_step(ranks == 1 ? 2 : ranks == 3 ? 4 : 3, simulation_case);
            check_simulation(8, simulation_case, 0, MPI_COMM_WORLD);
            check_simulation(8, simulation_case, 11, reversed);
            check_simulation(13, simulation_case, 12, MPI_COMM_WORLD);
            check_simulation(ranks == 1 ? 2 : ranks == 3 ? 4 : 3, simulation_case, 9, MPI_COMM_WORLD);
        }
        try {
            const MpiBlock invalid(ranks == 1 ? 1 : 2, MPI_COMM_WORLD);
            throw std::runtime_error("Accepted a block with no interior cells");
        } catch (const std::invalid_argument&) {
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    MPI_Comm_free(&reversed);
    MPI_Finalize();
}
