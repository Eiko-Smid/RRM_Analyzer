
#pragma once

#include <cstddef>
#include <cstdint>

namespace rrm
{
    /** @brief Stores an extreme value and its timestamp. */
    struct Extreme
    {
        double value{0.0};
        std::int64_t time_ms{0};
    };

    /** @brief Stores statistical results for one measurement quantity. */
    struct QuantityStats
    {
        double mean{0.0};
        Extreme min;
        Extreme max;
    };

    /** @brief Stores the analysis results for a selected measurement step. */
    struct MeasurementAnalysis
    {
        int step{0};
        std::size_t sample_count{0};

        std::int64_t start_time_ms{0};
        std::int64_t duration_ms{0};

        QuantityStats radial_force;
        QuantityStats torque;
    };

} // namespace rrm
