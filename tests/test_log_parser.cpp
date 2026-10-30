#include "rrm/rrm_log_parser.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace
{
    namespace fs = std::filesystem;

    constexpr char DEFAULT_LOG_PATH[] = "data/202211302134.log";
    constexpr int TEST_STEP = 5;

    /** @brief Fails the test when a condition is not satisfied. */
    void require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
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

    /** @brief Creates a temporary directory and removes it after testing. */
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            const auto unique_id =
                std::chrono::steady_clock::now().time_since_epoch().count();

            path_ = fs::temp_directory_path() /
                ("rrm_parser_tests_" + std::to_string(unique_id));
            fs::create_directories(path_);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            fs::remove_all(path_, error);
        }

        TemporaryDirectory(const TemporaryDirectory&) = delete;
        TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

        /** @brief Writes a complete fixture and returns its file path. */
        [[nodiscard]] fs::path write(
            const std::string& name,
            const std::string& contents
        ) const
        {
            const fs::path file_path = path_ / name;
            std::ofstream output(file_path, std::ios::binary);

            if (!output || !(output << contents))
            {
                throw std::runtime_error(
                    "Cannot create test log: " + file_path.string()
                );
            }

            output.close();
            require(output.good(), "Cannot finish writing test log.");
            return file_path;
        }

        [[nodiscard]] fs::path path() const { return path_; }

    private:
        fs::path path_;
    };

    /** @brief Tests successful parsing, step selection, and parser reuse. */
    void testValidLog(const fs::path& file_path)
    {
        std::cout << "\n--- Test 1: Complete log and step filtering ---\n";

        rrm::RrmLogParser parser;
        require(!parser.hasLoaded(), "A new parser must be empty.");

        // Load only step 5 and verify the resulting measurements.
        parser.load(file_path.string(), TEST_STEP);
        require(parser.hasLoaded(), "Parser should contain loaded data.");

        const auto& selected = parser.getData();
        require(selected.parse_status == rrm::ParseStatus::Complete,
                "Valid log must be complete.");
        require(selected.sel_step && *selected.sel_step == TEST_STEP,
                "Selected step was not recorded.");
        require(selected.n_meas_rows == 4, "Total row count is incorrect.");
        require(selected.meas_samples.size() == 2,
                "Step filter returned the wrong number of samples.");
        require(selected.meas_samples[0].step == TEST_STEP &&
                    selected.meas_samples[0].time_ms == 2000 &&
                    selected.meas_samples[0].radial_force == 10.0 &&
                    selected.meas_samples[0].torque == 4.0,
                "First selected measurement is incorrect.");
        require(selected.meas_samples[1].time_ms == 3000,
                "Second selected timestamp is incorrect.");
        require(selected.selected_step_finished,
                "Selected step should be marked as finished.");
        require(selected.metadata.at("station:") == "1",
                "Metadata was not loaded correctly.");
        require(selected.summary.at("log stopped:") == "yes",
                "Summary was not loaded correctly.");

        // Reuse the parser: std::nullopt must load all measurement steps.
        parser.load(file_path.string(), std::nullopt);
        const auto& all_steps = parser.getData();
        require(all_steps.parse_status == rrm::ParseStatus::Complete,
                "All-step parsing failed.");
        require(!all_steps.sel_step,
                "All-step mode must not contain a selected step.");
        require(all_steps.meas_samples.size() == all_steps.n_meas_rows &&
                    all_steps.meas_samples.size() == 4,
                "All-step mode must retain every measurement.");
        require(all_steps.meas_samples.front().step == 2 &&
                    all_steps.meas_samples.back().step == 6,
                "Measurements are not in the expected order.");

        // Clearing must remove data and make getData() invalid.
        parser.clear();
        require(!parser.hasLoaded(), "clear() did not reset the parser.");
        requireThrows<std::logic_error>(
            [&parser] { (void)parser.getData(); },
            "getData() must throw when no data has been loaded."
        );

        std::cout << "PASSED: Complete parsing, filtering, and clearing.\n";
    }

    /** @brief Tests rejection of a step that does not occur in the file. */
    void testMissingStep(const fs::path& file_path)
    {
        std::cout << "\n--- Test 2: Unavailable step ---\n";

        rrm::RrmLogParser parser;
        requireThrows<std::runtime_error>(
            [&] { parser.load(file_path.string(), 99); },
            "An unavailable measurement step must be rejected."
        );
        require(!parser.hasLoaded(),
                "Parser must clear its data after an unavailable step.");

        std::cout << "PASSED: Unavailable step rejected.\n";
    }

    /** @brief Verifies that an early EOF preserves partial measurements. */
    void testIncompleteLog(const fs::path& file_path)
    {
        std::cout << "\n--- Test 3: Incomplete log ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path.string(), TEST_STEP);
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Incomplete,
                "Unexpected EOF must produce Incomplete status.");
        require(!data.parse_message.empty(),
                "Incomplete parsing must provide an explanation.");
        require(data.n_meas_rows == 1 && data.meas_samples.size() == 1,
                "Partial measurements were not preserved.");
        require(data.meas_samples.front().time_ms == 1000,
                "Preserved sample is incorrect.");

        std::cout << "PASSED: Incomplete status and partial results.\n";
    }

    /** @brief Verifies strict measurement row-width validation. */
    void testMalformedMeasurement(const fs::path& file_path)
    {
        std::cout << "\n--- Test 4: Invalid measurement row ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path.string(), TEST_STEP);
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Error,
                "Malformed measurement must produce Error status.");
        require(data.parse_message.find("Invalid measurement column count") !=
                    std::string::npos,
                "Error message must identify the invalid column count.");
        require(data.meas_samples.size() == 1 && data.n_meas_rows == 1,
                "Measurements preceding an invalid row must be retained.");

        std::cout << "PASSED: Invalid row detected; earlier data retained.\n";
    }

    /** @brief Verifies that the completion marker is mandatory. */
    void testMissingCompletionMarker(const fs::path& file_path)
    {
        std::cout << "\n--- Test 5: Missing completion marker ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path.string(), TEST_STEP);
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Incomplete,
                "Missing completion marker must produce Incomplete status.");
        require(data.parse_message.find("log stopped:") != std::string::npos,
                "Missing completion marker must be explained.");
        require(data.meas_samples.size() == 2,
                "Measurements must survive an incomplete summary.");

        std::cout << "PASSED: Missing completion marker detected.\n";
    }

    /** @brief Verifies support for a caller-supplied field delimiter. */
    void testCustomDelimiter(const fs::path& file_path)
    {
        std::cout << "\n--- Test 6: Custom delimiter ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path.string(), TEST_STEP, "|");
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Complete &&
                    data.meas_samples.size() == 2,
                "Custom delimiter was not handled correctly.");

        std::cout << "PASSED: Custom delimiter.\n";
    }

    /** @brief Verifies that configuration and file errors are reported. */
    void testInputErrors(
        const fs::path& valid_log,
        const fs::path& missing_log
    )
    {
        std::cout << "\n--- Test 7: File and configuration errors ---\n";

        rrm::RrmLogParser parser;
        parser.load(missing_log.string(), TEST_STEP);
        require(parser.hasLoaded() &&
                    parser.getData().parse_status == rrm::ParseStatus::Error,
                "Missing file must produce Error status.");

        parser.load(valid_log.string(), TEST_STEP, "");
        require(parser.getData().parse_status == rrm::ParseStatus::Error,
                "Empty delimiter must produce Error status.");
        require(parser.getData().parse_message.find("Delimiter") !=
                    std::string::npos,
                "Invalid delimiter must be explained.");

        std::cout << "PASSED: Input errors correctly reported.\n";
    }

    /** @brief Runs an additional integration test with an actual RRM log. */
    void testRealLog(const std::string& file_path)
    {
        std::cout << "\n--- Test 8: Real-log integration ---\n";

        rrm::RrmLogParser parser;
        parser.load(file_path, TEST_STEP);
        const auto& data = parser.getData();

        require(data.parse_status == rrm::ParseStatus::Complete,
                "Real log was not parsed completely: " + data.parse_message);
        require(!data.meas_samples.empty(),
                "Real log contains no samples for step 5.");
        require(data.n_meas_rows >= data.meas_samples.size(),
                "Selected sample count exceeds the total measurement rows.");

        for (const auto& sample : data.meas_samples)
        {
            require(sample.step == TEST_STEP,
                    "Real-log step filtering is incorrect.");
        }

        std::cout << "Rows: " << data.n_meas_rows
                  << ", selected samples: " << data.meas_samples.size()
                  << "\nPASSED: Real-log integration.\n";
    }
} // namespace

int main(int argc, char* argv[])
{
    const std::string real_log = argc > 1 ? argv[1] : DEFAULT_LOG_PATH;

    try
    {
        TemporaryDirectory fixtures;

        // The header and every measurement row have exactly five fields.
        const fs::path valid_log = fixtures.write("valid.log",
            "station:;1\n"
            "Count;time;step;radial load;torque\n"
            "1;1000;2;12.0;3.0\n"
            "2;2000;5;10.0;4.0\n"
            "3;3000;5;20.0;6.0\n"
            "4;4000;6;18.0;5.0\n"
            ";\n"
            "log stopped:;yes\n");

        const fs::path incomplete_log = fixtures.write("incomplete.log",
            "station:;1\n"
            "Count;time;step;radial load;torque\n"
            "1;1000;5;10.0;4.0\n");

        // The second row omits only the unused final column. Even though
        // all required values are present, strict width validation must fail.
        const fs::path malformed_log = fixtures.write("malformed.log",
            "station:;1\n"
            "Count;time;step;radial load;torque;unused\n"
            "1;1000;5;10.0;4.0;ok\n"
            "2;2000;5;20.0;6.0\n"
            ";\n"
            "log stopped:;yes\n");

        const fs::path missing_marker_log = fixtures.write("missing_marker.log",
            "station:;1\n"
            "Count;time;step;radial load;torque\n"
            "1;1000;5;10.0;4.0\n"
            "2;2000;5;20.0;6.0\n"
            ";\n"
            "summary:;present\n");

        const fs::path custom_delimiter_log = fixtures.write("custom.log",
            "station:|1\n"
            "Count|time|step|radial load|torque\n"
            "1|1000|5|10.0|4.0\n"
            "2|2000|5|20.0|6.0\n"
            "|\n"
            "log stopped:|yes\n");

        testValidLog(valid_log);
        testMissingStep(valid_log);
        testIncompleteLog(incomplete_log);
        testMalformedMeasurement(malformed_log);
        testMissingCompletionMarker(missing_marker_log);
        testCustomDelimiter(custom_delimiter_log);
        testInputErrors(valid_log, fixtures.path() / "does_not_exist.log");
        testRealLog(real_log);

        std::cout << "\nAll parser tests passed!\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "\nTEST FAILED: " << error.what() << '\n';
        return 1;
    }
}
