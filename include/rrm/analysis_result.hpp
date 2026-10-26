#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <unordered_map>


namespace rrm
{
    struct Extreme
    {
        double value;
        std::int64_t time_ms;
    };

    struct QuantityStats
    {
        double mean;
        Extreme min;
        Extreme max;
    };

    struct WarmupAnalysis
    {
        double duration_ms;
        std::size_t sample_count;

        QuantityStats radial_force;
        QuantityStats torque;
    };
    
}