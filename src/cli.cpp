#include "cli.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
template <typename Number>
Number parse_number(std::string_view text, std::string_view option) {
    Number value{};
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
        if (text.empty() || text.front() == '-') {
            throw std::invalid_argument(std::string(option) + " requires a valid number");
        }
    }
    const auto end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::invalid_argument(std::string(option) + " requires a valid, representable number");
    }
    return value;
}

SimulationCase parse_case(std::string_view value) {
    if (value == "1") return SimulationCase::FourHotEdges;
    if (value == "2") return SimulationCase::HotTopAndBottom;
    if (value == "V") return SimulationCase::Verification;
    throw std::invalid_argument("--case must be 1, 2, or V");
}
}

Configuration parse_arguments(int argc, char* argv[]) {
    std::map<std::string_view, std::string_view> options{
        {"--case", {}}, {"--N", {}}, {"--r", {}}, {"--steps", {}},
        {"--T", {}}, {"--decomp", {}}, {"--out", {}}
    };
    for (int i = 1; i < argc; ++i) {
        const std::string_view name = argv[i];
        const auto option = options.find(name);
        if (option == options.end()) {
            throw std::invalid_argument("Unknown option " + std::string(name));
        }
        if (!option->second.empty()) {
            throw std::invalid_argument("Duplicate option " + std::string(name));
        }
        if (i + 1 == argc || argv[i + 1][0] == '\0' ||
            std::string_view(argv[i + 1]).substr(0, 2) == "--") {
            throw std::invalid_argument("Missing value for " + std::string(name));
        }
        option->second = argv[++i];
    }

    for (const auto name : {"--case", "--N", "--r"}) {
        if (options.at(name).empty()) {
            throw std::invalid_argument("Missing required option " + std::string(name));
        }
    }
    const bool fixed_steps = !options.at("--steps").empty();
    const bool requested_time = !options.at("--T").empty();
    if (fixed_steps == requested_time) {
        throw std::invalid_argument("Specify exactly one of --steps or --T");
    }
    const auto decomp = options.at("--decomp");
    if (decomp == "strips" || decomp == "blocks") {
        throw std::invalid_argument(std::string(decomp) + " decomposition is not implemented yet");
    }
    if (!decomp.empty() && decomp != "serial") {
        throw std::invalid_argument("--decomp must be serial");
    }

    Configuration config{parse_case(options.at("--case")),
                         parse_number<int>(options.at("--N"), "--N"),
                         parse_number<double>(options.at("--r"), "--r"),
                         0, std::string(options.at("--out"))};
    if (config.N <= 1) {
        throw std::invalid_argument("--N must be greater than 1");
    }
    const auto side = static_cast<std::size_t>(config.N) + 1;
    if (side > std::vector<double>().max_size() / side) {
        throw std::invalid_argument("--N is too large for a temperature grid");
    }
    if (!(config.r > 0.0 && config.r <= 0.25)) {
        throw std::invalid_argument("--r must be finite and satisfy 0 < r <= 0.25");
    }

    const double h = 1.0 / config.N;
    const double h_squared = h * h;
    if (config.r * h_squared == 0.0) {
        throw std::invalid_argument("--r and --N give a time step too small to represent");
    }
    if (fixed_steps) {
        config.steps = parse_number<int>(options.at("--steps"), "--steps");
        if (config.steps < 0) {
            throw std::invalid_argument("--steps must be non-negative");
        }
        return config;
    }

    const double final_time = parse_number<double>(options.at("--T"), "--T");
    if (!std::isfinite(final_time) || final_time < 0.0) {
        throw std::invalid_argument("--T must be finite and non-negative");
    }
    if (final_time == 0.0) {
        return config;
    }

    // Use extra precision when counting steps near an integer or the int limit.
    const long double max_dt = static_cast<long double>(config.r) * h_squared;
    const long double steps = std::ceil(static_cast<long double>(final_time) / max_dt);
    if (steps > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("--T requires more time steps than supported");
    }
    config.steps = static_cast<int>(steps);
    const long double target_dt = static_cast<long double>(final_time) / config.steps;
    // Cap the rounded coefficient so it cannot exceed the requested stability limit.
    config.r = std::min(config.r, static_cast<double>(target_dt / h_squared));
    const double actual_dt = config.r * h_squared;
    const double actual_time = config.steps * actual_dt;
    constexpr double time_tolerance = 4.0 * std::numeric_limits<double>::epsilon();
    if (std::abs(actual_time - final_time) / final_time > time_tolerance) {
        throw std::invalid_argument("--T cannot be reached with a representable stable time step");
    }
    return config;
}
