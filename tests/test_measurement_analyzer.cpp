#include "rrm/measurement_analyzer.hpp"
#include "rrm/rrm_log_parser.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    constexpr char DEFAULT_LOG_PATH[] = "data/202211302134.log";
    constexpr int TEST_STEP = 5;
    constexpr double TOLERANCE = 1e-9;

    /** @brief Fails the test when a condition is not satisfied. */
    void require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    /** @brief Checks floating-point results using an absolute tolerance. */
    void requireNear(
        double actual,
        double expected,
        const std::string& metric,
        double tolerance = TOLERANCE
    )
    {
        if (!std::isfinite(actual) || !std::isfinite(expected) ||
            std::abs(actual - expected) > tolerance)
        {
            throw std::runtime_error(
                metric + ": expected " + std::to_string(expected) +
                ", got " + std::to_string(actual)
            );
        }
    }

    /** @brief Verifies that an operation throws the expected exception. */
    template <typename Exception, typename Function>
    void requireThrows(Function&& operation, const std::string& message)
    {
        try
        {
            std::forward<Function>(operation)();
        }
        catch (const Exception&)
        {
            return;
        }

        throw std::runtime_error(message);
    }

    /** @brief Creates known measurements, including other steps to filter out. */
    [[nodiscard]] rrm::LogData makeTestData()
    {
        rrm::LogData data{};
        data.parse_status = rrm::ParseStatus::Complete;
        data.sel_step = std::nullopt;

        // MeasurementSample fields: step, time_ms, radial_force, torque.
        data.meas_samples = {
            {2,  500, 100.0, 100.0},
            {5, 1000,  10.0,   3.0},
            {5, 2000,  30.0,   7.0},
            {5, 4000,  20.0,   5.0},
            {6, 5000, 100.0, 100.0}
        };

        return data;
    }

    /** @brief Compares all analyzer outputs with independently known values. */
    void testKnownStatistics()
    {
        std::cout << "\n--- Test 1: Known measurement statistics ---\n";

        const rrm::LogData data = makeTestData();
        const rrm::MeasurementAnalyzer analyzer;
        const auto result = analyzer.analyzeMeasurements(data, TEST_STEP);

        // Other steps must not influence sample count or time calculations.
        require(result.step == TEST_STEP, "Analyzed step is incorrect.");
        require(result.sample_count == 3, "Sample count is incorrect.");
        require(result.start_time_ms == 1000, "Start time is incorrect.");
        require(result.duration_ms == 3000, "Duration is incorrect.");

        // Radial force: 10, 30, 20 -> mean 20, min 10, max 30.
        requireNear(result.radial_force.mean, 20.0, "Radial force mean");
        requireNear(result.radial_force.min.value, 10.0, "Radial force min");
        requireNear(result.radial_force.max.value, 30.0, "Radial force max");
        require(result.radial_force.min.time_ms == 1000,
                "Radial force minimum timestamp is incorrect.");
        require(result.radial_force.max.time_ms == 2000,
                "Radial force maximum timestamp is incorrect.");

        // Torque: 3, 7, 5 -> mean 5, min 3, max 7.
        requireNear(result.torque.mean, 5.0, "Torque mean");
        requireNear(result.torque.min.value, 3.0, "Torque min");
        requireNear(result.torque.max.value, 7.0, "Torque max");
        require(result.torque.min.time_ms == 1000,
                "Torque minimum timestamp is incorrect.");
        require(result.torque.max.time_ms == 2000,
                "Torque maximum timestamp is incorrect.");

        std::cout << "PASSED: All computed metrics match expected values.\n";
    }

    /** @brief Rejects incomplete or invalid parser results. */
    void testParseStatusValidation()
    {
        std::cout << "\n--- Test 2: Parse status validation ---\n";

        rrm::LogData data = makeTestData();
        const rrm::MeasurementAnalyzer analyzer;

        data.parse_status = rrm::ParseStatus::Incomplete;
        requireThrows<std::runtime_error>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Incomplete log data must be rejected."
        );

        data.parse_status = rrm::ParseStatus::Error;
        requireThrows<std::runtime_error>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Invalid log data must be rejected."
        );

        std::cout << "PASSED: Incomplete and invalid data rejected.\n";
    }

    /** @brief Rejects analysis of a step excluded by the parser. */
    void testStepSelectionValidation()
    {
        std::cout << "\n--- Test 3: Step selection validation ---\n";

        rrm::LogData data = makeTestData();
        data.sel_step = 2;
        const rrm::MeasurementAnalyzer analyzer;

        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "A step not loaded by the parser must be rejected."
        );

        std::cout << "PASSED: Unavailable selected step rejected.\n";
    }

    /** @brief Rejects data sets with fewer than two selected samples. */
    void testInsufficientSamples()
    {
        std::cout << "\n--- Test 4: Insufficient samples ---\n";

        rrm::LogData data = makeTestData();
        data.meas_samples = {{5, 1000, 10.0, 3.0}};
        const rrm::MeasurementAnalyzer analyzer;

        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "A single measurement must be rejected."
        );

        data.meas_samples.clear();
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "An empty data set must be rejected."
        );

        std::cout << "PASSED: Insufficient samples rejected.\n";
    }

    /** @brief Rejects negative timestamps and non-finite measurements. */
    void testInvalidMeasurementValues()
    {
        std::cout << "\n--- Test 5: Invalid measurement values ---\n";

        rrm::LogData data = makeTestData();
        const rrm::MeasurementAnalyzer analyzer;

        data.meas_samples[1].time_ms = -1;
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Negative timestamps must be rejected."
        );

        data = makeTestData();
        data.meas_samples[1].radial_force =
            std::numeric_limits<double>::quiet_NaN();
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "NaN radial force must be rejected."
        );

        data = makeTestData();
        data.meas_samples[1].torque =
            std::numeric_limits<double>::infinity();
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Infinite torque must be rejected."
        );

        std::cout << "PASSED: Invalid measurements rejected.\n";
    }

    /** @brief Rejects unordered timestamps and a zero-duration step. */
    void testTimeValidation()
    {
        std::cout << "\n--- Test 6: Timestamp validation ---\n";

        rrm::LogData data = makeTestData();
        const rrm::MeasurementAnalyzer analyzer;

        // The third selected sample must not precede the second one.
        data.meas_samples[3].time_ms = 1500;
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Out-of-order timestamps must be rejected."
        );

        // Equal timestamps are permitted, but total duration must be > 0.
        data = makeTestData();
        data.meas_samples[2].time_ms = 1000;
        data.meas_samples[3].time_ms = 1000;
        requireThrows<std::invalid_argument>(
            [&] { (void)analyzer.analyzeMeasurements(data, TEST_STEP); },
            "Zero measurement duration must be rejected."
        );

        std::cout << "PASSED: Invalid measurement times rejected.\n";
    }

    /** @brief Tests parsing a real log and analyzing its selected samples. */
    void testIntegration(const std::string& file_path)
    {
        std::cout << "\n--- Test 7: Parser + analyzer integration ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path, TEST_STEP);
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Complete,
                "Real log parsing failed: " + data.parse_message);
        require(data.meas_samples.size() >= 2,
                "Real log contains too few samples for analysis.");

        for (const auto& sample : data.meas_samples)
        {
            require(sample.step == TEST_STEP,
                    "Parser returned a measurement from the wrong step.");
        }

        const rrm::MeasurementAnalyzer analyzer;
        const auto result = analyzer.analyzeMeasurements(data, TEST_STEP);

        // Confirm consistency between parsed samples and analyzer output.
        require(result.step == TEST_STEP, "Analyzed step is incorrect.");
        require(result.sample_count == data.meas_samples.size(),
                "Analyzer sample count differs from parser output.");
        require(result.start_time_ms == data.meas_samples.front().time_ms,
                "Analyzer start time differs from the first sample.");
        require(result.duration_ms ==
                    data.meas_samples.back().time_ms -
                    data.meas_samples.front().time_ms,
                "Analyzer duration differs from parsed timestamps.");
        require(result.duration_ms > 0, "Duration must be positive.");

        // All summary values and extrema timestamps must be plausible.
        const auto checkStats = [&](const rrm::QuantityStats& stats,
                                    const std::string& name)
        {
            require(std::isfinite(stats.mean) &&
                        std::isfinite(stats.min.value) &&
                        std::isfinite(stats.max.value),
                    name + " contains non-finite statistics.");
            require(stats.min.value <= stats.mean &&
                        stats.mean <= stats.max.value,
                    name + " statistics are inconsistent.");

            const auto end_time_ms =
                result.start_time_ms + result.duration_ms;
            require(stats.min.time_ms >= result.start_time_ms &&
                        stats.min.time_ms <= end_time_ms &&
                        stats.max.time_ms >= result.start_time_ms &&
                        stats.max.time_ms <= end_time_ms,
                    name + " extrema timestamps are outside the step.");
        };

        checkStats(result.radial_force, "Radial force");
        checkStats(result.torque, "Torque");

        std::cout << "Samples: " << result.sample_count
                  << "\nDuration: " << result.duration_ms / 60000.0 << " min\n"
                  << "\nRadial force:\n"
                  << "Mean: " << result.radial_force.mean << '\n'
                  << "Min:  " << result.radial_force.min.value << '\n'
                  << "Max:  " << result.radial_force.max.value << '\n'
                  << "\nTorque:\n"
                  << "Mean: " << result.torque.mean << '\n'
                  << "Min:  " << result.torque.min.value << '\n'
                  << "Max:  " << result.torque.max.value << '\n'
                  << "\nPASSED: Integration test.\n";
    }
} // namespace

int main(int argc, char* argv[])
{
    const std::string file_path = argc > 1 ? argv[1] : DEFAULT_LOG_PATH;

    try
    {
        testKnownStatistics();
        testParseStatusValidation();
        testStepSelectionValidation();
        testInsufficientSamples();
        testInvalidMeasurementValues();
        testTimeValidation();
        testIntegration(file_path);

        std::cout << "\nAll analyzer tests passed!\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "\nTEST FAILED: " << error.what() << '\n';
        return 1;
    }
}
