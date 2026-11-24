#pragma once

#include "grid.hpp"
#include "simulation_case.hpp"

#include <mpi.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class MpiBlock {
public:
    // Collective on parent. Destroy the block before MPI_Finalize.
    MpiBlock(int intervals, MPI_Comm parent);
    ~MpiBlock();
    MpiBlock(const MpiBlock&) = delete;
    MpiBlock& operator=(const MpiBlock&) = delete;

    MPI_Comm communicator() const { return cart_; }
    int rank() const { return rank_; }
    // Cartesian axes are {row, column}, increasing upward and rightward.
    const std::array<int, 2>& dimensions() const { return dimensions_; }
    const std::array<int, 2>& coordinates() const { return coordinates_; }
    int first_global_row() const { return first_row_; }
    int first_global_column() const { return first_column_; }
    int owned_rows() const { return rows_; }
    int owned_columns() const { return columns_; }
    int bottom_neighbor() const { return bottom_; }
    int top_neighbor() const { return top_; }
    int left_neighbor() const { return left_; }
    int right_neighbor() const { return right_; }

    std::size_t local_rows() const { return static_cast<std::size_t>(rows_) + 2; }
    std::size_t local_columns() const { return static_cast<std::size_t>(columns_) + 2; }
    std::size_t size() const { return temperatures_.size(); }
    // Owned cells start at local (row 1, column 1). A halo face at an
    // MPI_PROC_NULL neighbor lies on the physical boundary.
    double* data() { return temperatures_.data(); }
    const double* data() const { return temperatures_.data(); }

    // Exchange owned edge cells with Cartesian neighbors. Initialize physical
    // boundary halos beforehand. Corner halo cells are untouched.
    void exchange_halos();

    // Refresh current halos and initialize physical boundaries in both blocks first.
    // Writes only the owned cells of a distinct destination with the same bounds.
    void heat_step(MpiBlock& next, double r) const;

    // Collective on the Cartesian communicator. Only Cartesian rank 0 gets a Grid.
    std::optional<Grid> gather_global_block_field(SimulationCase simulation_case) const;

private:
    MPI_Comm cart_ = MPI_COMM_NULL;
    std::array<int, 2> dimensions_{};
    std::array<int, 2> coordinates_{};
    int rank_;
    int intervals_;
    int first_row_;
    int first_column_;
    int rows_;
    int columns_;
    int bottom_;
    int top_;
    int left_;
    int right_;
    std::vector<double> temperatures_;
};

struct MpiBlockResult {
    std::unique_ptr<MpiBlock> block;
    double dt;
    double final_time;
    double t_total;
    double t_comp;
    double t_comm;
};

// Collective on parent with identical inputs. Destroy the result before
// MPI_Finalize. Owned cells and physical boundaries hold final values.
MpiBlockResult run_mpi_block(int N, SimulationCase simulation_case, double r, int steps,
                             MPI_Comm parent);
