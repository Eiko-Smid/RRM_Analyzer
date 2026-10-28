#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <optional>


namespace rrm
{
    // Status of the complete log-file parsing operation.
    enum class ParseStatus
    {
        Complete,
        Incomplete,
        Error
    };

    struct MeasurementSample
    {
        // Keep the step so samples can be distinguished when all steps are loaded.
        std::int64_t step{0};
        std::int64_t time_ms{0};
        double radial_force{0.0};
        double torque{0.0};
    };

    struct LogData
    {
        // Stores filepath
        std::string file_path;
        // Defines which data rows to load (sort by step column)
        // std::nullopt means that all measurement steps will be loaded.
        std::optional<int> sel_step{5};

        // Define storage for other log file sections

        // Storage for metadata
        std::unordered_map<std::string, std::string> metadata;
        // Storage for summary
        std::unordered_map<std::string, std::string> summary;

        std::vector<std::string> metadata_notes;
        std::vector<std::string> summary_notes;

        // Storage vector for relevant measurements
        std::vector<MeasurementSample> meas_samples;

        // Stores total numbers of measurement table
        std::size_t n_meas_rows{0};

        // File completion and diagnostic information.
        ParseStatus parse_status{ParseStatus::Incomplete};
        std::string parse_message;

        // Indicates whether a later step or the table end was observed
        // after the requested step. This is not proof of data integrity.
        bool selected_step_finished{false};
    };
} // namespace rrm
