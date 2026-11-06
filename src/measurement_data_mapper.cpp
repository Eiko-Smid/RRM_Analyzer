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
        const ColumnLogData& source
    )
    {
        // Copy information shared by the generic and legacy data structures.
        LogData legacy_data{};
        legacy_data.file_path = source.file_path;
        legacy_data.sel_step = source.sel_step;
        legacy_data.metadata = source.metadata;
        legacy_data.summary = source.summary;
        legacy_data.metadata_notes = source.metadata_notes;
        legacy_data.summary_notes = source.summary_notes;
        legacy_data.n_meas_rows = source.n_meas_rows;
        legacy_data.parse_status = source.parse_status;
        legacy_data.parse_message = source.parse_message;
        legacy_data.selected_step_finished = source.selected_step_finished;

        // Preserve parser diagnostics when parsing failed before a header was found.
        if (source.data_table.columns.empty() &&
            source.parse_status != ParseStatus::Complete)
        {
            return legacy_data;
        }

        // Find and validate every column required by MeasurementAnalyzer.
        const Column& step_column = requiredColumn(
            source, "step", ValueType::Int64
        );
        const Column& time_column = requiredColumn(
            source, "time", ValueType::Int64
        );
        const Column& force_column = requiredColumn(
            source, "radial load", ValueType::Double
        );
        const Column& torque_column = requiredColumn(
            source, "torque", ValueType::Double
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
        legacy_data.meas_samples.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            legacy_data.meas_samples.push_back({
                requiredValue<std::int64_t>(step_column, i),
                requiredValue<std::int64_t>(time_column, i),
                requiredValue<double>(force_column, i),
                requiredValue<double>(torque_column, i)
            });
        }

        return legacy_data;
    }
} // namespace rrm
