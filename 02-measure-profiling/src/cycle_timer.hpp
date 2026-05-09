#pragma once

#include <cstdint>

/** Serialization barriers to prevent instruction reordering across measurement boundaries. */
class ReorderBarrier
{
public:
    /**
     * cpuid is a fully-serializing instruction: nothing before or after it can reorder past it.
     * Used here as a measurement fence, not to read CPU identification data.
     */
    static void cpuid() noexcept
    {
        asm volatile("cpuid" ::: "rax", "rbx", "rcx", "rdx");
    }

    /** lfence serializes loads only; insufficient when store reordering must also be prevented. */
    static void lfence() noexcept
    {
        asm volatile("lfence" ::: "memory");
    }
};

class CycleTimer
{
public:
    using value_t = std::uint64_t;

    static value_t rdtsc() noexcept
    {
        std::uint32_t lo, hi;
        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
        return (static_cast<value_t>(hi) << 32) | lo;
    }

    static value_t rdtscp() noexcept
    {
        std::uint32_t lo, hi;
        asm volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
        return (static_cast<value_t>(hi) << 32) | lo;
    }

    void start() noexcept
    {
        ReorderBarrier::cpuid(); // serialize before sampling the counter
        _start = rdtsc();
    }

    [[nodiscard]] value_t stop() noexcept
    {
        ReorderBarrier::cpuid(); // ensure the workload retires before reading the counter
        const auto end = rdtscp();

        return end - _start;
    }

private:
    value_t _start;
};
