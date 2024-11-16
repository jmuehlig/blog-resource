#include <iostream>
#include <cstdint>
#include <vector>
#include <random>
#include <argparse/argparse.hpp>
#include <perfcpp/event_counter.h>
#include <perfcpp/hardware_info.h>

/**
 * Pump a single value to an entire cache line (64B).
 */
struct alignas(64U) cache_line {
    std::uint64_t value;
};

int main(int count_arguments, char **arguments) {
    auto argument_parser = argparse::ArgumentParser{"random-access-bench"};

    argument_parser.add_argument("--perf")
            .help("Use perf-cpp.")
            .implicit_value(true)
            .default_value(false);

    argument_parser.add_argument("--size")
            .help("Number of items to access.")
            .default_value(std::uint64_t(16777216)) /* 1GB */
            .action([](const std::string &value) { return std::uint64_t(std::stoull(value)); });

    /// Parse arguments.
    try {
        argument_parser.parse_args(count_arguments, arguments);
    } catch (std::runtime_error &e) {
        std::cout << argument_parser << std::endl;
        return 1;
    }

    const auto is_count_events = argument_parser.get<bool>("--perf");

    /// Setup random access benchmark: Create data to access.
    auto cache_lines = std::vector<cache_line>{};
    cache_lines.resize(argument_parser.get<std::uint64_t>("--size"));
    for (auto& cache_line : cache_lines) {
        cache_line.value = std::uintptr_t(&cache_line) - std::uintptr_t(&cache_lines);
    }

    /// Setup random access benchmark: Create access pattern.
    auto access_pattern_indices = std::vector<std::uint64_t>{};
    access_pattern_indices.resize(cache_lines.size());
    std::iota(access_pattern_indices.begin(), access_pattern_indices.end(), 0U);
    std::shuffle(access_pattern_indices.begin(), access_pattern_indices.end(), std::mt19937{std::random_device{}()});

    /// Setup perf-cpp.
    auto counter_definitions = perf::CounterDefinition{};
    auto event_counter = perf::EventCounter{counter_definitions};
    if (is_count_events) {
        try {
            event_counter.add({"instructions", "cycles", "", "L1-dcache-loads", "L1-dcache-load-misses"});

            if (perf::HardwareInfo::is_amd()) {
                counter_definitions.add("CYCLES_NO_RETIRE.NOT_COMPLETE_MISSING_LOAD", 0x53a2d6);
                event_counter.add("CYCLES_NO_RETIRE.NOT_COMPLETE_MISSING_LOAD");
            }

            event_counter.start();
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << std::endl;
        }
    }

    /// Process the data and force the value to be not optimized away by the compiler.
    auto sum = 0ULL;
    for (const auto index: access_pattern_indices) {
        sum += cache_lines[index].value;
    }
    asm volatile("" : : "r,m"(sum) : "memory");

    if (is_count_events) {
        /// Stop recording counters and get the result (normalized to the number of accessed cache lines).
        event_counter.stop();
        const auto result = event_counter.result();

        /// Print the performance counters.
        for (const auto [name, value]: result) {
            std::cout << std::uint64_t(value) << " " << name << " (" << value / double(cache_lines.size()) << " per access)"
                      << std::endl;
        }
    }

    return 0;
}
