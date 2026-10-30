
#pragma once

#include "rrm/log_data.hpp"
#include "rrm/analysis_result.hpp"

#include <cstdint>

namespace rrm
{
    /**
     * @brief Computes statistical metrics for measurement data.
     *
     * Analyzes a selected measurement step and calculates the
     * duration, mean, minimum and maximum values of radial
     * force and torque, including timestamps of the extrema.
     */
    class MeasurementAnalyzer
    {
    public:

        /** @brief Default constructor. */
        MeasurementAnalyzer() = default;

        /**
         * @brief Analyzes measurements of a selected step while checking for complete log data.
         *
         * @param data Parsed log data containing measurement samples.
         * @param step Measurement step to analyze. Default is 5.
         * @return MeasurementAnalysis containing the computed metrics.
         */
        [[nodiscard]] MeasurementAnalysis analyzeMeasurements(
            const LogData& data,
            int step = 5
        ) const;

    private:
        /**
         * @brief Processes samples and computes the desired metrics which are min, max and mean
         * of the torque and radial force.
         * @param data Parsed log data containing measurement samples.
         * @param step Step to process.
         * @return Computed measurement metrics.
         */
        [[nodiscard]] static MeasurementAnalysis processMeasurements(
            const LogData& data,
            int step
        );

        /**
         * @brief Updates the minimum and maximum of a quantity.
         *
         * @param stats Statistics structure to update.
         * @param value Measurement step to analyze. Default is 5.
         * @param time_ms Timestamp of the current measurement.
         */
        static void updateExtrema(
            QuantityStats& stats,
            double value,
            std::int64_t time_ms
        ) noexcept;

    };

} // namespace rrm
