#include "binary_field.hpp"
#include "cli.hpp"
#include "serial.hpp"
#include "verification.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "Usage: heat --case 1|2|V --N N --r R (--steps COUNT | --T TIME)\n"
                     "            [--decomp serial] [--out FILE]\n"
                     "  --N        Grid intervals per axis, greater than 1\n"
                     "  --r        Stability parameter, 0 < R <= 0.25\n"
                     "  --steps    Non-negative number of steps using R\n"
                     "  --T        Non-negative final time using R as an upper limit\n"
                     "  --decomp   Decomposition mode (default: serial)\n"
                     "  --out      Write the final field as raw little-endian doubles\n"
                     "  --help     Show this usage information\n";
        return 0;
    }

    try {
        const Configuration config = parse_arguments(argc, argv);
        const auto result = run_serial(config.N, config.simulation_case, config.r, config.steps);
        if (!config.output_filename.empty()) {
            write_binary_field(result.grid, config.output_filename);
        }
        const char* case_name = config.simulation_case == SimulationCase::FourHotEdges ? "1" :
                                config.simulation_case == SimulationCase::HotTopAndBottom ? "2" : "V";
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
                  << "decomp=serial P=1 case=" << case_name << " N=" << config.N
                  << " steps=" << config.steps << " r=" << config.r
                  << " dt=" << result.dt << " T=" << result.final_time
                  << " t_total=" << result.computation_seconds
                  << " t_comp=" << result.computation_seconds << " t_comm=0";
        if (config.simulation_case == SimulationCase::Verification) {
            const auto errors = verification_errors(result.grid, result.final_time);
            std::cout << " err_inf=" << errors.infinity << " err_l2=" << errors.l2;
        }
        std::cout << '\n';
    } catch (const std::bad_alloc&) {
        std::cerr << "Error: insufficient memory for the requested simulation\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\nRun heat --help for usage.\n";
        return 1;
    }
}
