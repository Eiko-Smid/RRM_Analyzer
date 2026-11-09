#include "rrm/measurement_data_mapper.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>


namespace rrm
{
    // -----------------------------------------------------------------------------------
    // Define helper functions
    // -----------------------------------------------------------------------------------

    namespace
    {
        /**
         * @brief Finds a required column and validates its declared value type.
         * @param source Column-oriented data containing the measurement columns.
         * @param name Exact, case-sensitive name of the required column.
         * @param expected_type Value type required by the legacy data structure.
         * @return Reference to the matching column.
         * @throws std::runtime_error If the column is missing or has the wrong type.
         */
        [[nodiscard]] const Column& requiredColumn(
            const ColumnLogData& source,
            const char* name,
            ValueType expected_type
        )
        {
            const auto column = std::find_if(
                source.data_table.columns.begin(),
                source.data_table.columns.end(),
                [name](const Column& candidate)
                {
                    return candidate.spec.name == name;
                }
            );

            if (column == source.data_table.columns.end())
            {
                throw std::runtime_error(
                    std::string("Required measurement column '") + name +
                    "' is missing."
                );
            }

            if (column->spec.type != expected_type)
            {
                throw std::runtime_error(
                    std::string("Measurement column '") + name +
                    "' has an unexpected stored type."
                );
            }

            return *column;
        }

        /**
         * @brief Reads a value from a column using the required variant type.
         * @tparam T Expected type stored in CellValue.
         * @param column Column containing the requested value.
         * @param row Zero-based index of the requested row.
         * @return The typed value at the requested row.
         * @throws std::runtime_error If the stored variant has an unexpected type.
         */
        template <typename T>
        [[nodiscard]] T requiredValue(
            const Column& column,
            std::size_t row
        )
        {
            const auto* value = std::get_if<T>(&column.values[row]);
            if (!value)
            {
                throw std::runtime_error(
                    "Measurement column '" + column.spec.name +
                    "' contains an unexpected value type at row " +
                    std::to_string(row) + "."
                );
            }

            return *value;
        }
    } // namespace


    // -----------------------------------------------------------------------------------
    // Implement class MeasurementDataMapper
    // -----------------------------------------------------------------------------------

    // ------------------------------------------------------------
    // Implement Public methods
    // ------------------------------------------------------------

    LogData MeasurementDataMapper::toLegacyData(
        const ColumnLogData& col_log_data
    )
    {
        // Copy data that both structures have in common
        LogData log_data{};
        log_data.file_path = col_log_data.file_path;
        log_data.sel_step = col_log_data.sel_step;
        log_data.metadata = col_log_data.metadata;
        log_data.summary = col_log_data.summary;
        log_data.metadata_notes = col_log_data.metadata_notes;
        log_data.summary_notes = col_log_data.summary_notes;
        log_data.n_meas_rows = col_log_data.n_meas_rows;
        log_data.parse_status = col_log_data.parse_status;
        log_data.parse_message = col_log_data.parse_message;
        log_data.selected_step_finished = col_log_data.selected_step_finished;

        // Retrun diagnostic and measurement data if table columns are empty.
        if (col_log_data.data_table.columns.empty() &&
            col_log_data.parse_status != ParseStatus::Complete)
        {
            return log_data;
        }

        // Find and validate every column required by MeasurementAnalyzer.
        const Column& step_column = requiredColumn(
            col_log_data, "step", ValueType::Int64
        );
        const Column& time_column = requiredColumn(
            col_log_data, "time", ValueType::Int64
        );
        const Column& force_column = requiredColumn(
            col_log_data, "radial load", ValueType::Double
        );
        const Column& torque_column = requiredColumn(
            col_log_data, "torque", ValueType::Double
        );

        // All required columns must describe the same number of measurements.
        const std::size_t count = step_column.values.size();
        if (time_column.values.size() != count ||
            force_column.values.size() != count ||
            torque_column.values.size() != count)
        {
            throw std::runtime_error(
                "Required measurement columns have inconsistent row counts."
            );
        }

        // Convert each column-oriented row into one legacy measurement sample.
        log_data.meas_samples.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            log_data.meas_samples.push_back({
                requiredValue<std::int64_t>(step_column, i),
                requiredValue<std::int64_t>(time_column, i),
                requiredValue<double>(force_column, i),
                requiredValue<double>(torque_column, i)
            });
        }

        return log_data;
    }
} // namespace rrm
