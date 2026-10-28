#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <unordered_map>


namespace rrm
{
    struct MeasurementSample
    {
        std::int64_t time_ms{0};
        double radial_force{0.0};
        double torque{0.0};
    };

    struct LogData
    {
        // Stores filepath
        std::string file_path;
        // Defines which data rows to load (sort by step column)
        int sel_step{5};

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


    };
} // namespace rrm