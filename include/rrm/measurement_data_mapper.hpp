#pragma once

#include "rrm/log_data.hpp"

namespace rrm
{
    /**
     * @brief Converts generic column-oriented log data into legacy measurement data.
     *
     * The mapper provides the boundary between RrmLogParser's generic storage and
     * the MeasurementSample storage consumed by MeasurementAnalyzer and the GUI.
     */
    class MeasurementDataMapper
    {
    public:
        /**
         * @brief Maps parsed measurement columns to legacy measurement samples.
         *
         * Required columns are located by their exact, case-sensitive names and
         * validated for the expected value types and consistent row counts.
         * Parser diagnostics are preserved when parsing failed before a measurement
         * header was found.
         *
         * @param source Generic column-oriented log data to convert.
         * @return LogData containing copied parser information and mapped samples.
         * @throws std::runtime_error If a required column is missing, has an
         *         unexpected type, or contains an inconsistent number of rows.
         */
        [[nodiscard]] static LogData toLegacyData(
            const ColumnLogData& source
        );
    };
} // namespace rrm
