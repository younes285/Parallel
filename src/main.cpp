#include "binary_field.hpp"
#include "cli.hpp"
#include "serial.hpp"
#include "verification.hpp"

#ifdef HEAT_WITH_MPI
#include "mpi_strip.hpp"
#include "mpi_block.hpp"
#endif

#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <algorithm>
#include <string_view>
#include <stdexcept>

namespace {
void print_summary(const Configuration& config, int ranks, double dt, double final_time,
                   double t_total, double t_comp, double t_comm, const Grid* field) {
    const char* case_name = config.simulation_case == SimulationCase::FourHotEdges ? "1" :
                            config.simulation_case == SimulationCase::HotTopAndBottom ? "2" : "V";
    std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
              << "decomp=" << (config.decomposition == Decomposition::Blocks ? "2d" :
                                config.decomposition == Decomposition::Strips ? "1d" : "serial")
              << " P=" << ranks << " case=" << case_name << " N=" << config.N
              << " steps=" << config.steps << " r=" << config.r
              << " dt=" << dt << " T=" << final_time
              << " t_total=" << t_total
              << " t_comp=" << t_comp << " t_comm=" << t_comm;
    if (config.simulation_case == SimulationCase::Verification) {
        const auto errors = verification_errors(*field, final_time);
        std::cout << " err_inf=" << errors.infinity << " err_l2=" << errors.l2;
    }
    std::cout << '\n';
}

#ifdef HEAT_WITH_MPI
template <typename Result>
int finish_mpi_run(const Configuration& config, int ranks, const Result& result,
                   const std::optional<Grid>& field, MPI_Comm comm) {
    const double local[] = {result.t_total, result.t_comp, result.t_comm};
    double maximum[3]{};
    MPI_Reduce(local, maximum, 3, MPI_DOUBLE, MPI_MAX, 0, comm);

    int status = 0;
    if (field) {
        try {
            if (!config.output_filename.empty()) {
                write_binary_field(*field, config.output_filename);
            }
            print_summary(config, ranks, result.dt, result.final_time,
                          maximum[0], maximum[1], maximum[2], &*field);
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << '\n';
            status = 1;
        }
    }
    MPI_Bcast(&status, 1, MPI_INT, 0, comm);
    return status;
}

int run_mpi_cli(int argc, char* argv[]) {
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr << "Error: MPI initialization failed\n";
        return 1;
    }
    int rank;
    int ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);

    Configuration config{};
    int status = 0;
    int parameters[5]{};
    if (rank == 0) {
        try {
            config = parse_arguments(argc, argv);
            if (config.decomposition == Decomposition::Strips && config.N - 1 < ranks) {
                throw std::invalid_argument("1d decomposition requires at least one interior row per rank");
            }
            if (config.decomposition == Decomposition::Blocks) {
                int dims[2]{};
                MPI_Dims_create(ranks, 2, dims);
                const int interior = config.N - 1;
                if (interior < std::max(dims[0], dims[1])) {
                    throw std::invalid_argument("2d decomposition requires at least one interior cell per axis per rank");
                }
                if (interior > std::numeric_limits<int>::max() / interior) {
                    throw std::length_error("MPI block gather exceeds the MPI count or displacement limit");
                }
            }
            parameters[0] = config.N;
            parameters[1] = config.steps;
            parameters[2] = static_cast<int>(config.simulation_case);
            parameters[3] = static_cast<int>(config.decomposition);
            parameters[4] = static_cast<int>(config.output_filename.size());
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << "\nRun heat --help for usage.\n";
            status = 1;
        }
    }
    MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (status != 0) {
        MPI_Finalize();
        return status;
    }
    MPI_Bcast(parameters, 5, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&config.r, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    config.N = parameters[0];
    config.steps = parameters[1];
    config.simulation_case = static_cast<SimulationCase>(parameters[2]);

    config.decomposition = static_cast<Decomposition>(parameters[3]);

    try {
        if (config.decomposition == Decomposition::Blocks) {
            // Cartesian rank zero may differ from world rank zero.
            config.output_filename.resize(parameters[4]);
            MPI_Bcast(config.output_filename.data(), parameters[4], MPI_CHAR, 0, MPI_COMM_WORLD);
            const auto result = run_mpi_block(config.N, config.simulation_case, config.r,
                                              config.steps, MPI_COMM_WORLD);
            const auto field = result.block->gather_global_block_field(config.simulation_case);
            status = finish_mpi_run(config, ranks, result, field, result.block->communicator());
        } else {
            const auto result = run_mpi_strip(config.N, config.simulation_case, config.r,
                                              config.steps, MPI_COMM_WORLD);
            const auto field = result.strip.gather_global_field(config.simulation_case);
            status = finish_mpi_run(config, ranks, result, field, MPI_COMM_WORLD);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error on rank " << rank << ": " << error.what() << '\n';
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    MPI_Finalize();
    return status;
}
#endif
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "Usage: heat --case 1|2|V --N N --r R (--steps COUNT | --T TIME)\n"
                     "            [--decomp serial|1d|2d] [--out FILE]\n"
                     "  --N        Grid intervals per axis, greater than 1\n"
                     "  --r        Stability parameter, 0 < R <= 0.25\n"
                     "  --steps    Non-negative number of steps using R\n"
                     "  --T        Non-negative final time using R as an upper limit\n"
                     "  --decomp   Decomposition mode: serial (default), 1d (MPI strips), or 2d (MPI blocks)\n"
                     "  --out      Write the final field as raw little-endian doubles\n"
                     "  --help     Show this usage information\n";
        return 0;
    }

#ifdef HEAT_WITH_MPI
    for (int i = 1; i < argc - 1; ++i) {
        const std::string_view mode = argv[i + 1];
        if (std::string_view(argv[i]) == "--decomp" && (mode == "1d" || mode == "2d")) {
            return run_mpi_cli(argc, argv);
        }
    }
#endif
    try {
        const Configuration config = parse_arguments(argc, argv);
        if (config.decomposition != Decomposition::Serial) {
            throw std::invalid_argument("This heat build has no MPI support");
        }
        const auto result = run_serial(config.N, config.simulation_case, config.r, config.steps);
        if (!config.output_filename.empty()) {
            write_binary_field(result.grid, config.output_filename);
        }
        print_summary(config, 1, result.dt, result.final_time, result.computation_seconds,
                      result.computation_seconds, 0.0, &result.grid);
    } catch (const std::bad_alloc&) {
        std::cerr << "Error: insufficient memory for the requested simulation\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\nRun heat --help for usage.\n";
        return 1;
    }
}
