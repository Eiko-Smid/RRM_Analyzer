#include <rrm/rrm_log_parser.hpp>

#include <exception>
#include <iostream>
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
        const int step = (argc > 2) ? std::stoi(argv[2]) : 5;

        // Create parser and load selected log file
        rrm::RrmLogParser parser;
        parser.load(file_path, step);

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
                << step << '\n';
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}