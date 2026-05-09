#include "result.hpp"
#include <sstream>
#include <tabulate/table.hpp>

std::string ResultSet::to_string()
{
    auto table = tabulate::Table{};

    table.add_row({
        "# run", "Is Rand. Access", "", "Measure Type", "kB", "cache lines", "ms", "cycles", "instructions",
        "L1-dcache-loads", "branches"
    });

    table.format()
    .border_top(" ")
    .border_bottom(" ")
    .border_left(" ")
    .border_right(" ")
    .corner(" ");

    for (const auto& result : this->_results)
    {
        auto row = tabulate::RowStream{};
        row << result.run() << result.is_random() << "" << Result::to_string(result.measure_type()) << result.
            workload_size_in_kb() << result.workload_size_in_number_of_cache_lines();

        if (result.milliseconds().has_value())
        {
            row << result.milliseconds().value() / result.workload_size_in_number_of_cache_lines();
        }
        else
        {
            row << "";
        }

        if (result.cycles().has_value())
        {
            row << result.cycles().value() / result.workload_size_in_number_of_cache_lines();
        }
        else
        {
            row << "";
        }

        if (result.instructions().has_value())
        {
            row << result.instructions().value() / result.workload_size_in_number_of_cache_lines();
        }
        else
        {
            row << "";
        }

        if (result.l1_dcache_loads().has_value())
        {
            row << result.l1_dcache_loads().value() / result.workload_size_in_number_of_cache_lines();
        }
        else
        {
            row << "";
        }

        if (result.branches().has_value())
        {
            row << result.branches().value() / result.workload_size_in_number_of_cache_lines();
        }
        else
        {
            row << "";
        }

        table.add_row(std::move(row));
    }

    return table.str();
}

std::string ResultSet::to_csv()
{
    auto stream = std::ostringstream{};

    stream << "run,is_random,measure_type,kb,cache_lines,ms_per_cache_line,cycles_per_cache_line,instructions_per_cache_line,L1-dcache-loads_per_cache_line,branches_per_cache_line\n";

    for (const auto& result : this->_results)
    {
        const auto n = static_cast<double>(result.workload_size_in_number_of_cache_lines());

        stream << result.run() << ","
               << result.is_random() << ","
               << Result::to_string(result.measure_type()) << ","
               << result.workload_size_in_kb() << ","
               << result.workload_size_in_number_of_cache_lines() << ",";

        result.milliseconds().has_value() ? stream << result.milliseconds().value() / n : stream << "";
        stream << ",";
        result.cycles().has_value() ? stream << result.cycles().value() / n : stream << "";
        stream << ",";
        result.instructions().has_value() ? stream << result.instructions().value() / n : stream << "";
        stream << ",";
        result.l1_dcache_loads().has_value() ? stream << result.l1_dcache_loads().value() / n : stream << "";
        stream << ",";
        result.branches().has_value() ? stream << result.branches().value() / n : stream << "";
        stream << "\n";
    }

    return stream.str();
}
