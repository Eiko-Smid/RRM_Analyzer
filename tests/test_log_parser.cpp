#include <rrm/rrm_log_parser.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <string>


// Define constants
namespace
{
    constexpr char DEFAULT_LOG_PATH[] = "data/202211302134.log";
}


int main(int argc, char* argv[])
{
    const std::string file_path = (argc > 1)
        ? argv[1] : DEFAULT_LOG_PATH;

    try
    {
        // A step number filters measurements; "all" selects every step.
        std::optional<int> step = 5;
        if (argc > 2)
        {
            const std::string step_arg = argv[2];
            step = step_arg == "all" ? std::nullopt
                                     : std::optional<int>(std::stoi(step_arg));
        }

        // Optional third argument selects a nondefault delimiter.
        const std::string delimiter = argc > 3 ? argv[3] : ";";

        // Create parser and load selected log file
        rrm::RrmLogParser parser;
        parser.load(file_path, step, delimiter);

        // Verify that data has been loaded
        if (!parser.hasLoaded())
        {
            std::cerr << "No data loaded.\n";
            return 1;
        }

        // Print metadata, first 10 samples and summary
        parser.display_data(10);

        // Access loaded data
        const auto& data = parser.getData();

        if (data.meas_samples.empty())
        {
            std::cerr
                << "Warning: No samples found for step "
                << (step ? std::to_string(*step) : "all") << '\n';
        }

        // A partial result stays available even when the file is incomplete.
        return data.parse_status == rrm::ParseStatus::Complete ? 0 : 2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
