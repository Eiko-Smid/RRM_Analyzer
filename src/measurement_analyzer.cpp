
#include "rrm/measurement_analyzer.hpp"

#include <cmath>
#include <stdexcept>


namespace rrm
{

    // -----------------------------------------------------------------------------------
    // Implement class MeasurementAnalyzer
    // -----------------------------------------------------------------------------------    

    // ------------------------------------------------------------
    // Implement Public methods
    // ------------------------------------------------------------


    MeasurementAnalysis MeasurementAnalyzer::analyzeMeasurements(
        const LogData& data,
        int step
    ) const
    {
        // Ensure the logfile was parsed completely.
        if (data.parse_status != ParseStatus::Complete)
        {
            throw std::runtime_error(
                "Cannot analyze incomplete or invalid log data: " +
                data.parse_message
            );
        }

        // Ensure the requested step is available from the parser.
        if (data.sel_step && *data.sel_step != step)
        {
            throw std::invalid_argument(
                "Requested step was not loaded by the parser."
            );
        }
        
        // Process measurement samples.
        return processMeasurements(
            data,
            step
        );
    }


    // ------------------------------------------------------------
    // Implement Private methods
    // ------------------------------------------------------------

    MeasurementAnalysis MeasurementAnalyzer::processMeasurements(
            const LogData& data,
            int step
    )
    {
        // Init result storage.
        MeasurementAnalysis result{};
        result.step = step;

        // Variables for mean calculation.
        double radial_force_sum = 0.0;
        double torque_sum = 0.0;

        // Timestamp of the previously processed sample.
        std::int64_t last_time_ms = 0;

        // Process measurement samples.
        for (const auto& sample : data.meas_samples)
        {
            // Skip measurement belonging to other steps.
            if (sample.step != step)
            {
                continue;
            }

            // Validate measurement values and timestamps.
            if (
                sample.time_ms < 0 ||
                !std::isfinite(sample.radial_force) ||
                !std::isfinite(sample.torque)
            )
            {
                throw std::invalid_argument(
                    "Selected measurements contain invalid values."
                );
            }

            // Init statistics using the first selected sample.
            if (result.sample_count == 0)
            {
                result.start_time_ms = sample.time_ms;

                result.radial_force.min = {
                    sample.radial_force, sample.time_ms
                };

                result.radial_force.max = {
                    sample.radial_force, sample.time_ms
                };

                result.torque.min = {
                    sample.torque, sample.time_ms
                };

                result.torque.max = {
                    sample.torque, sample.time_ms
                };
            }
            else if (sample.time_ms < last_time_ms)
            {
                throw std::invalid_argument(
                    "Measurement timestamps are not ordered."
                );
            }

            // Update radial force statistics.
            updateExtrema(
                result.radial_force,
                sample.radial_force,
                sample.time_ms
            );

            // Update torque statistics.
            updateExtrema(
                result.torque,
                sample.torque,
                sample.time_ms
            );

            // sum up values for mean calculation.
            radial_force_sum += sample.radial_force;
            torque_sum += sample.torque;

            // Update number of processed samples and timestamp.
            ++result.sample_count;
            last_time_ms = sample.time_ms;
        }

        // Ensure that enough measurements are available.
        if (result.sample_count < 2)
        {
            throw std::invalid_argument(
                "At least two samples are required for analysis."
            );
        }

        // Calculate duration of the selected measurement step.
        result.duration_ms = last_time_ms - result.start_time_ms;

        if (result.duration_ms <= 0)
        {
            throw std::invalid_argument(
                "Measurement duration must be greater than zero."
            );
        }

        // Calculate mean values.
        const double n_samples =
            static_cast<double>(result.sample_count);

        result.radial_force.mean = radial_force_sum / n_samples;
        result.torque.mean = torque_sum / n_samples;

        return result;
    }


    void MeasurementAnalyzer::updateExtrema(
        QuantityStats& stats,
        double value,
        std::int64_t time_ms
    ) noexcept
    {
        // Update minimum.
        if (value < stats.min.value)
        {
            stats.min = {value, time_ms};
        }

        // Update maximum.
        if (value > stats.max.value)
        {
            stats.max = {value, time_ms};
        }
    }

} // namespace rrm
