#pragma once
#include <optional>
#include <cstdint>
#include <vector>
#include <string>

class ResultSet
{
public:
    class Result
    {
    public:
        enum MeasureType
        {
            Cycles,
            EventCounter,
            LiveEventCounter
        };

        Result(const std::uint32_t run, const bool is_random, const MeasureType measure_type,
               const std::size_t workload_size_in_kb, const std::size_t workload_size_in_number_of_cache_lines,
               const std::optional<double> milliseconds, const std::optional<double> cycles,
               const std::optional<double> instructions, const std::optional<double> l1_dcache_loads,
               const std::optional<double> branches) noexcept
            : _run(run), _is_random(is_random), _measure_type(measure_type), _workload_size_in_kb(workload_size_in_kb),
              _workload_size_in_number_of_cache_lines(workload_size_in_number_of_cache_lines),
              _milliseconds(milliseconds), _cycles(cycles), _instructions(instructions),
              _l1_dcache_loads(l1_dcache_loads), _branches(branches)
        {
        }

        ~Result() noexcept = default;

        [[nodiscard]] std::uint32_t run() const noexcept { return _run; }
        [[nodiscard]] bool is_random() const noexcept { return _is_random; }
        [[nodiscard]] MeasureType measure_type() const noexcept { return _measure_type; }
        [[nodiscard]] std::size_t workload_size_in_kb() const noexcept { return _workload_size_in_kb; }

        [[nodiscard]] std::size_t workload_size_in_number_of_cache_lines() const noexcept
        {
            return _workload_size_in_number_of_cache_lines;
        }

        [[nodiscard]] std::optional<double> milliseconds() const noexcept { return _milliseconds; }
        [[nodiscard]] std::optional<double> cycles() const noexcept { return _cycles; }
        [[nodiscard]] std::optional<double> instructions() const noexcept { return _instructions; }
        [[nodiscard]] std::optional<double> l1_dcache_loads() const noexcept { return _l1_dcache_loads; }
        [[nodiscard]] std::optional<double> branches() const noexcept { return _branches; }

        [[nodiscard]] static std::string to_string(const MeasureType measure_type)
        {
            switch (measure_type)
            {
            case MeasureType::Cycles: return "cycles";
            case MeasureType::EventCounter: return "EventCounter";
            case MeasureType::LiveEventCounter: return "LiveEventCounter";
            default: return "Unknown";
            }
        }

    private:
        std::uint32_t _run;
        bool _is_random;
        MeasureType _measure_type;
        std::size_t _workload_size_in_kb;
        std::size_t _workload_size_in_number_of_cache_lines;

        std::optional<double> _milliseconds;
        std::optional<double> _cycles;
        std::optional<double> _instructions;
        std::optional<double> _l1_dcache_loads;
        std::optional<double> _branches;
    };

    void push_back(Result result)
    {
        _results.push_back(result);
    }

    [[nodiscard]] std::string to_string();
    [[nodiscard]] std::string to_csv();

private:
    std::vector<Result> _results;
};
