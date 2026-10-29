
#include "rrm/rrm_log_parser.hpp"
#include "rrm/measurement_analyzer.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>


// ------------------------------------------------------------
// Global defs
// ------------------------------------------------------------
namespace
{   
    // Define default path of log file to load for integration test.
    constexpr char DEFAULT_LOG_PATH[] = "data/202211302134.log";
}

// ------------------------------------------------------------
// Test helpers
// ------------------------------------------------------------

/**
 * @brief Checks whether two double values are equal within a certain threshold.
 *
 * @param actual Actual computed value.
 * @param expected Expected reference value.
 * @param threshold Maximum allowed absolute difference. Default is 1e-9.
 */
void checkEqualDouble(
    double actual,
    double expected,
    double threshold = 1e-9
)
{
    // Validate threshold.
    if (!std::isfinite(threshold) || threshold < 0.0)
    {
        throw std::invalid_argument(
            "Threshold must be finite and non-negative."
        );
    }

    // Check if values are equal within the threshold.
    if (
        !std::isfinite(actual) ||
        !std::isfinite(expected) ||
        std::abs(actual - expected) > threshold
    )
    {
        throw std::runtime_error(
            "Test failed: Expected " + std::to_string(expected) +
            ", got " + std::to_string(actual) +
            ", threshold: " + std::to_string(threshold)
        );
    }
}


// ------------------------------------------------------------
// Test 1: Analyzer with known measurements
// ------------------------------------------------------------

/**
 * @brief Tests the analyzer using manually defined data anc compares it against epected
 * results
 * 
 */
void testMeasAnalyzer()
{
    std::cout << "\n--- Test 1: Analyzer ---\n";
    
    // Define expected results
    // Define step and sample count    
    constexpr int STEP = 5;
    constexpr std::size_t EXPECTED_SAMPLE_COUNT = 3;

    // Expected timestamps and duration.
    constexpr std::int64_t EXPECTED_START_TIME_MS = 1000;
    constexpr std::int64_t EXPECTED_DURATION_MS = 3000;

    // Expected radial force statistics.
    constexpr double EXPECTED_RADIAL_MEAN = 20.0;
    constexpr double EXPECTED_RADIAL_MIN = 10.0;
    constexpr double EXPECTED_RADIAL_MAX = 30.0;

    constexpr std::int64_t EXPECTED_RADIAL_MIN_TIME_MS = 1000;
    constexpr std::int64_t EXPECTED_RADIAL_MAX_TIME_MS = 2000;

    // Expected torque statistics.
    constexpr double EXPECTED_TORQUE_MEAN = 5.0;
    constexpr double EXPECTED_TORQUE_MIN = 3.0;
    constexpr double EXPECTED_TORQUE_MAX = 7.0;

    constexpr std::int64_t EXPECTED_TORQUE_MIN_TIME_MS = 1000;
    constexpr std::int64_t EXPECTED_TORQUE_MAX_TIME_MS = 2000;

    // Simulate data
    // Define simulated measurements.
    rrm::LogData data;
    data.parse_status = rrm::ParseStatus::Complete;
    data.sel_step = std::nullopt;

    data.meas_samples = {
        // step, time_ms, radial_force, torque
        {2,  500, 100.0, 100.0},
        {5, 1000,  10.0,   3.0},
        {5, 2000,  30.0,   7.0},
        {5, 4000,  20.0,   5.0},
        {6, 5000, 100.0, 100.0}
    };

    // Analyze step 5.
    rrm::MeasurementAnalyzer analyzer;
    const auto result = analyzer.analyzeMeasurements(data, STEP);

    // Verify results
    // Verify general metrics.
    if (result.step != STEP ||
        result.sample_count != EXPECTED_SAMPLE_COUNT)
    {
        throw std::runtime_error("Step or sample count incorrect.");
    }

    // Verify time calculations.
    if (result.start_time_ms != EXPECTED_START_TIME_MS ||
        result.duration_ms != EXPECTED_DURATION_MS)
    {
        throw std::runtime_error("Time calculation incorrect.");
    }

    // Verify radial force statistics.
    checkEqualDouble(result.radial_force.mean, EXPECTED_RADIAL_MEAN);
    checkEqualDouble(result.radial_force.min.value, EXPECTED_RADIAL_MIN);
    checkEqualDouble(result.radial_force.max.value, EXPECTED_RADIAL_MAX);

    // Verify torque statistics.
    checkEqualDouble(result.torque.mean, EXPECTED_TORQUE_MEAN);
    checkEqualDouble(result.torque.min.value, EXPECTED_TORQUE_MIN);
    checkEqualDouble(result.torque.max.value, EXPECTED_TORQUE_MAX);

    // Verify extrema timestamps.
    if (
        result.radial_force.min.time_ms != EXPECTED_RADIAL_MIN_TIME_MS ||
        result.radial_force.max.time_ms != EXPECTED_RADIAL_MAX_TIME_MS ||
        result.torque.min.time_ms != EXPECTED_TORQUE_MIN_TIME_MS ||
        result.torque.max.time_ms != EXPECTED_TORQUE_MAX_TIME_MS
    )
    {
        throw std::runtime_error("Extrema timestamps incorrect.");
    }

    std::cout << "PASSED: All computed metrics are correct.\n";
}


// ------------------------------------------------------------
// Test 2: Parser and analyzer integration
// ------------------------------------------------------------

/** 
 * @brief Tests the analyzer using a real RRM logfile which is loaded by LogParser. No default
 * value comparison is performed for the computed metrics.
 * @param file_path Path to the logfile.
 */
void testIntegration(const std::string& file_path)
{
    std::cout << "\n--- Test 2: Parser + Analyzer ---\n";

    // Define 
    constexpr int STEP = 5;

    // Load measurement step 5.
    rrm::RrmLogParser parser;
    parser.load(file_path, STEP);

    const auto& data = parser.getData();

    // Verify parser output.
    if (data.parse_status != rrm::ParseStatus::Complete)
        throw std::runtime_error("Log parsing was not complete.");

    if (data.meas_samples.empty())
        throw std::runtime_error("No measurements loaded.");

    for (const auto& sample : data.meas_samples)
    {
        if (sample.step != STEP)
            throw std::runtime_error("Incorrect step loaded.");
    }

    // Compute metrics.
    rrm::MeasurementAnalyzer analyzer;
    const auto result = analyzer.analyzeMeasurements(data, STEP);

    // Verify consistency.
    if (result.sample_count != data.meas_samples.size())
        throw std::runtime_error("Sample count mismatch.");

    if (result.duration_ms <= 0)
        throw std::runtime_error("Invalid measurement duration.");

    // Display results.
    std::cout << "Samples: " << result.sample_count << '\n';
    std::cout << "Duration: "
              << result.duration_ms / 60000.0 << " min\n";

    std::cout << "\nRadial force:\n"
              << "Mean: " << result.radial_force.mean << '\n'
              << "Min:  " << result.radial_force.min.value << '\n'
              << "Max:  " << result.radial_force.max.value << '\n';

    std::cout << "\nTorque:\n"
              << "Mean: " << result.torque.mean << '\n'
              << "Min:  " << result.torque.min.value << '\n'
              << "Max:  " << result.torque.max.value << '\n';

    std::cout << "\nPASSED: Integration test completed.\n";
}


// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Choose path of logfile
    const std::string file_path = (argc > 1)
        ? argv[1]
        : DEFAULT_LOG_PATH;

    try
    {   
        // Test the measurement analyzer against expected results.
        testMeasAnalyzer();

        // Test meas analyzer with real data loaded by log parser.
        testIntegration(file_path);

        std::cout << "\nAll tests passed!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nTEST FAILED: " << e.what() << '\n';
        return 1;
    }
}
