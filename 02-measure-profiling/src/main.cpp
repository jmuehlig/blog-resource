#include <iostream>
#include <cstdint>
#include <utility>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <perfcpp/event_counter.hpp>
#include "result.hpp"
#include "cycle_timer.hpp"

/**
 * Pump a single value to an entire cache line (64B).
 */
struct alignas(64U) cache_line
{
    std::uint64_t value;
};

/**
 * Creates a set of indices (access pattern) and data (cache lines).
 *
 * @param is_random If set, access pattern will be random.
 * @param size_in_kb Size for the workload in KB.
 * @return Pair of access pattern and data.
 */
std::pair<std::vector<std::uint64_t>, std::vector<cache_line>>
create_workload(bool is_random, std::size_t size_in_kb);

/**
 * noinline: prevents the compiler from folding the loop body into the caller and changing the instruction mix.
 * no-tree-vectorize: keeps the loop scalar so per-iteration instruction counts match the assembly baseline.
 */
[[gnu::noinline, gnu::optimize("no-tree-vectorize")]]
std::uint64_t access_workload(const std::vector<std::uint64_t>& access_pattern, const std::vector<cache_line>& data);

int main()
{
    const auto sizes_in_kb = std::vector{
        1UL, 2UL, 4UL, 8UL, 16UL, 32UL, 64UL, 128UL, 256UL, 512UL, 1024UL, 2048UL, 4096UL, 8192UL, 16384UL, 32768UL,
        65536UL, 131072UL, 262144UL, 524288UL, 1048576UL, 2097152UL
    };

    constexpr auto runs = 4U;

    auto benchmark_result = ResultSet{};

    /// EventCounter
    for (const auto is_random : {true, false})
    {
        for (const auto size_in_kb : sizes_in_kb)
        {
            for (auto run_id = 0U; run_id < runs; ++run_id)
            {
                auto event_counter = perf::EventCounter{};
                event_counter.add({"ms", "cycles", "instructions", "L1-dcache-loads", "branches"});

                {
                    const auto [access_pattern, data] = create_workload(is_random, size_in_kb);

                    ReorderBarrier::cpuid();
                    event_counter.start();
                    const auto sum = access_workload(access_pattern, data);
                    asm volatile("" : : "r,m"(sum) : "memory"); // prevent dead-code elimination of the loop

                    ReorderBarrier::cpuid();
                    event_counter.stop();

                    const auto result = event_counter.result();
                    benchmark_result.push_back(ResultSet::Result{
                        run_id, is_random, ResultSet::Result::MeasureType::EventCounter, size_in_kb,
                        access_pattern.size(), result.get("ms"), result.get("cycles"), result.get("instructions"),
                        result.get("L1-dcache-loads"), result.get("branches")
                    });
                }
            }
        }
    }

    /// LiveEventCounter
    {
        auto event_counter = perf::EventCounter{};
        event_counter.add_live({"instructions", "cycles", "branches", "L1-dcache-loads"});

        // rdpmc reads a running hardware counter directly; the counter must be active before any LiveEventCounter samples it.
        event_counter.start();
        for (const auto is_random : {true, false})
        {
            for (const auto size_in_kb : sizes_in_kb)
            {
                for (auto run_id = 0U; run_id < runs; ++run_id)
                {
                    auto live_event_counter = perf::LiveEventCounter{event_counter};

                    {
                        const auto [access_pattern, data] = create_workload(is_random, size_in_kb);

                        ReorderBarrier::cpuid();
                        live_event_counter.start();
                        const auto sum = access_workload(access_pattern, data);
                        asm volatile("" : : "r,m"(sum) : "memory"); // prevent dead-code elimination of the loop

                        ReorderBarrier::cpuid();
                        live_event_counter.stop();

                        benchmark_result.push_back(ResultSet::Result{
                            run_id, is_random, ResultSet::Result::MeasureType::LiveEventCounter, size_in_kb,
                            access_pattern.size(), std::nullopt, live_event_counter.get("cycles"),
                            live_event_counter.get("instructions"), live_event_counter.get("L1-dcache-loads"),
                            live_event_counter.get("branches")
                        });
                    }
                }
            }
        }
        event_counter.stop();
    }

    /// Raw cycles
    for (const auto is_random : {true, false})
    {
        for (const auto size_in_kb : sizes_in_kb)
        {
            for (auto run_id = 0U; run_id < runs; ++run_id)
            {
                {
                    auto cycle_timer = CycleTimer{};
                    const auto [access_pattern, data] = create_workload(is_random, size_in_kb);

                    cycle_timer.start();
                    const auto sum = access_workload(access_pattern, data);
                    asm volatile("" : : "r,m"(sum) : "memory"); // prevent dead-code elimination of the loop
                    const auto cycles = cycle_timer.stop();

                    benchmark_result.push_back(ResultSet::Result{
                        run_id, is_random, ResultSet::Result::MeasureType::Cycles, size_in_kb,
                        access_pattern.size(), std::nullopt, cycles, std::nullopt,
                        std::nullopt, std::nullopt
                    });
                }
            }
        }
    }

    std::cout << benchmark_result.to_string() << std::endl;

    if (auto file = std::ofstream{"result.csv"}; file.is_open())
    {
        file << benchmark_result.to_csv();
    }

    return 0;
}

std::pair<std::vector<std::uint64_t>, std::vector<cache_line>>
create_workload(const bool is_random, const std::size_t size_in_kb)
{
    const auto number_of_cache_lines = (size_in_kb * 1024UL) / sizeof(cache_line);

    /// Setup random access benchmark: Create data to access.
    auto cache_lines = std::vector<cache_line>{};
    cache_lines.resize(number_of_cache_lines);
    for (auto& cache_line : cache_lines)
    {
        cache_line.value = std::uintptr_t(&cache_line) - std::uintptr_t(&cache_lines);
    }

    /// Setup random access benchmark: Create access pattern.
    auto access_pattern_indices = std::vector<std::uint64_t>{};
    access_pattern_indices.resize(cache_lines.size());
    std::iota(access_pattern_indices.begin(), access_pattern_indices.end(), 0U);

    if (is_random)
    {
        std::ranges::shuffle(access_pattern_indices, std::mt19937{std::random_device{}()});
    }

    return {std::move(access_pattern_indices), std::move(cache_lines)};
}

[[gnu::noinline, gnu::optimize("no-tree-vectorize")]]
std::uint64_t access_workload(const std::vector<std::uint64_t>& access_pattern, const std::vector<cache_line>& data)
{
    auto sum = 0ULL;
    for (const auto index : access_pattern)
    {
        sum += data[index].value;
    }
    return sum;
}
