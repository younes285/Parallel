#pragma once

#include "grid.hpp"
#include "simulation_case.hpp"

#include <mpi.h>

#include <cstddef>
#include <optional>
#include <vector>

class MpiStrip {
public:
    // Requires an initialized intracommunicator and at least one interior row per rank.
    MpiStrip(int intervals, MPI_Comm comm);

    int rank() const { return rank_; }
    int ranks() const { return ranks_; }
    int first_global_row() const { return first_global_row_; }
    int owned_rows() const { return owned_rows_; }
    int lower_neighbor() const { return lower_neighbor_; }
    int upper_neighbor() const { return upper_neighbor_; }
    std::size_t columns() const { return columns_; }
    std::size_t size() const { return temperatures_.size(); }

    // Local row 0 is the lower ghost. Row owned_rows()+1 is the upper ghost.
    // At the domain ends these slots represent global rows 0 and N.
    double* data() { return temperatures_.data(); }
    const double* data() const { return temperatures_.data(); }

    // Call on every rank using the communicator supplied at construction.
    // The communicator must remain valid for the duration of the exchange.
    void exchange_ghost_rows();

    // Initialize physical boundaries in both strips and refresh this strip's ghosts first.
    // Writes only the owned interior cells of a distinct strip with the same layout.
    void heat_step(MpiStrip& next, double r) const;

    // Collective on the borrowed communicator. Only rank zero receives a Grid.
    std::optional<Grid> gather_global_field(SimulationCase simulation_case) const;

private:
    MPI_Comm comm_;
    int rank_;
    int ranks_;
    int first_global_row_;
    int owned_rows_;
    int lower_neighbor_;
    int upper_neighbor_;
    std::size_t columns_;
    std::vector<double> temperatures_;
};

struct MpiStripResult {
    MpiStrip strip;
    double dt;
    double final_time;
    double t_total;
    double t_comp;
    double t_comm;
};

// All ranks call with the same inputs. Only owned rows and physical boundaries
// in the returned strip are current. The communicator is borrowed.
MpiStripResult run_mpi_strip(int N, SimulationCase simulation_case, double r, int steps,
                             MPI_Comm comm);
