#pragma once

#include "rrm/log_data.hpp"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>


namespace rrm
{
    class RrmLogParser
    {
        public:
            /** @brief Constructor */
            RrmLogParser() = default;

            /** @brief Loads the logfile from the given path into memory.
             * Selects only the rows that match current step value. 
             * @param file_path The path of the log file to load.
             * @param step The step to load or alternatively "std::nullopt" to load all steps.
             * @param delimiter String used to separate fields in each line of the log file.
            */
            void load(
                const std::string& file_path,
                std::optional<int> step = 5,
                const std::string& delimiter = ";"
            );

            /** @brief Displays n_rows of the loaded data.
             * @param n_rows The number of rows to display. Default is 10.
            */
            void display_data(std::size_t n_rows = 10) const;

            /** @brief Returns a reference to the loaded data.
             * @return A reference to the loaded LogData.
             */
            [[nodiscard]] const LogData& getData() const;

            /** @brief Checks if the logfile has been loaded.
             * @return true if the logfile has been loaded, false otherwise.
             */
            [[nodiscard]] bool hasLoaded() const noexcept;

            /** @brief Clears the loaded data from memory. */
            void clear() noexcept;

        private:

            // States of the state machine.
            enum class ParseState
            {
                // Idle state for resetting.
                IDLE,

                // State for reading metadata.
                READ_METADATA,

                // State for reading measurement table.
                READ_MEASUREMENTS,

                // State for reading summary table.
                READ_SUMMARY,

                // State that indicates that the parsing has completed successfully.
                COMPLETED,

                // State that indicates that an error has occurred during parsing and handles the error message.
                ERROR
            };

            // Represents the states of the file reading process.
            struct FileState
            {
                // Indicates that the end of file has been reached.
                bool eof_reached{false};
                // Indicates that a file reading error has occurred.
                bool read_err{false};
            };

            // Stores the column indices of the measurement table as well as the expected number of columns.
            struct ColInfo
            {
                std::size_t time_idx{0};
                std::size_t step_idx{0};
                std::size_t radial_force_idx{0};
                std::size_t torque_idx{0};

                // The number of column names that have been found in the measurement table.  
                std::size_t exp_n_columns{0};
            };

            ParseState state{ParseState::IDLE};
            FileState file_state;
            ColInfo col_info;

            // Indicates if measurement table has been found in log file.
            bool found_table{false};

            // Stores data while parsing the log file.
            LogData result;

            // Error handling flags and messages.
            bool parsing_error{false};
            std::string error_message;

            // Storage struct for the loaded log data
            std::optional<LogData> log_data_;

            // Flags set only by state processing; transitions only inspect them.
            bool meas_separator_found{false};
            bool log_stopped_found{false};
            bool selected_step_seen{false};


            // Define transitions
            /**
             * @brief Transitions from IDLE to READ_METADATA if the file is open
             *        and no parsing error has occurred.
             * @param f_opened Indicates whether the log file was opened successfully.
             */
            void transIdleToReadMeta(bool f_opened);

            /**
             * @brief Transitions from IDLE to ERROR if the file could not be opened
             *        or a parsing error has occurred.
             * @param f_opened Indicates whether the log file was opened successfully.
             */
            void transIdleToErr(bool f_opened);

            /**
             * @brief Transitions from READ_METADATA to READ_MEASUREMENTS when the
             *        measurement table header has been found without errors.
             */
            void transReadMetaToReadMeas();

            /**
             * @brief Transitions from READ_METADATA to ERROR if an unexpected EOF,
             *        file reading error, or parsing error occurs.
             */
            void transReadMetaToErr();

            /**
             * @brief Transitions from READ_MEASUREMENTS to READ_SUMMARY when the
             *        measurement table separator has been found.
             */
            void transReadMeasToReadSum();

            /**
             * @brief Transitions from READ_MEASUREMENTS to ERROR if an unexpected EOF,
             *        file reading error, or parsing error occurs.
             */
            void transReadMeasToErr();

            /**
             * @brief Transitions from READ_SUMMARY to COMPLETED when EOF is reached,
             *        the log completion marker was found, and no errors occurred.
             */
            void transReadSummaryCompleted();

            /**
             * @brief Transitions from READ_SUMMARY to ERROR if a file reading error,
             *        parsing error, or EOF without the log completion marker occurs.
             */
            void transReadSummaryToErr();


            // Define state processes that define the actions in each state 
            /**
             * @brief Initializes the parser state, resets internal data and error flags,
             *        and validates the file opening result and delimiter.
             * @param file_path Path to the log file.
             * @param step Optional measurement step to load. std::nullopt selects all steps.
             * @param delimiter String used to separate fields in each line of the log file.
             * @param f_opened Indicates whether the log file was opened successfully.
             */
            void processStateIdle(
                const std::string& file_path,
                std::optional<int> step,
                const std::string& delimiter,
                bool f_opened
            );

            /**
             * @brief Reads and stores metadata until the measurement table header is found.
             *        Extracts column indices required for parsing measurements.
             * @param input Input stream of the opened log file.
             * @param line_number Current line number, incremented after each successful read.
             * @param delimiter String used to separate fields in each line of the log file.
             */
            void processStateReadMeta(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter
            );

            /**
             * @brief Reads and validates measurement rows, stores samples matching the
             *        selected step, and detects the end of the measurement table.
             * @param input Input stream of the opened log file.
             * @param line_number Current line number, incremented after each successful read.
             * @param delimiter String used to separate fields in each line of the log file.
             * @param step Optional measurement step to load. std::nullopt selects all steps.
             */
            void processStateReadMeas(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter,
                std::optional<int> step
            );

            /**
             * @brief Reads and stores summary entries and detects the final
             *        log completion marker.
             * @param input Input stream of the opened log file.
             * @param line_number Current line number, incremented after each successful read.
             * @param delimiter String used to separate fields in each line of the log file.
             */
            void processStateReadSummary(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter
            );

            /**
             * @brief Marks the parsing result as complete and terminates the parser FSM.
             * @param finished Reference to the flag indicating whether parsing has finished.
             */
            void processStateCompleted(bool& finished);

            /**
             * @brief Marks the parsing result as incomplete or erroneous, stores the
             *        error message, and terminates the parser FSM.
             * @param finished Reference to the flag indicating whether parsing has finished.
             */
            void processStateErr(bool& finished);

            /**
             * Reads the next line from the input stream, updates the file state, and splits it into fields.
             * @param input The input stream to read from.
             * @param line_number A reference to the line number counter.
             * @param delimiter The delimiter to split the line by.
             * @param fields A reference to the vector to store the split fields.
             * @return True if a line was successfully read, false otherwise.
             */
            bool readNextLine(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter,
                std::vector<std::string>& fields
            );

            /**
             * @brief Loads the lof file of the given file path, for the given step or alternatively all steps.
             * @param file_path The path of the log file to load.
             * @param step The step to load or alternatively "std::nullopt" to load all steps.
             * @param delimiter String used to separate fields in each line of the log file.
             * @return Returns the LogData struct containing the content of the log file.
             */
            [[nodiscard]] LogData parsFile(
                const std::string& file_path,
                std::optional<int> step,
                const std::string& delimiter
            );
    };
} // namespace rrm
