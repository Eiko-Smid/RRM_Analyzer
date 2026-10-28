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
             * @param file_path Path to the logfile to load.
             * @param step The step value to filter the rows. Default is 5.
             *             std::nullopt loads all steps.
             * @param delimiter The delimiter to split the logfile lines into chunks.
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

            // States of the state machine
            enum class ParseState
            {
                IDLE,
                READ_METADATA,
                READ_MEASUREMENTS,
                READ_SUMMARY,
                COMPLETED,
                ERROR
            };

            struct FileState
            {
                bool eof_reached{false};
                bool read_err{false};
            };

            struct ColInfo
            {
                std::size_t time_idx{0};
                std::size_t step_idx{0};
                std::size_t radial_force_idx{0};
                std::size_t torque_idx{0};
                std::size_t exp_n_columns{0};
            };

            // Define transitions
            ParseState state{ParseState::IDLE};
            FileState file_state;
            ColInfo col_info;

            // Indicates if measurement table has been found in log file
            bool found_table{false};

            // Stores data while parsing the log file
            LogData result;

            bool parsing_error{false};
            std::string error_message;

            // Flags set only by state processing; transitions only inspect them.
            bool meas_separator_found{false};
            bool log_stopped_found{false};
            bool selected_step_seen{false};

            // Define transitions
            void transIdleToReadMeta(bool f_opened);

            void transIdleToErr(bool f_opened);
            
            void transReadMetaToReadMeas();

            void transReadMetaToErr();

            void transReadMeasToReadSum();

            void transReadMeasToErr();

            void transReadSummaryCompleted();

            void transReadSummaryToErr();

            // Reads one line, updates file-state flags and splits it into fields.
            // Called only from reading-state processing methods.
            bool readNextLine(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter,
                std::vector<std::string>& fields
            );

            void processStateIdle(
                const std::string& file_path,
                std::optional<int> step,
                const std::string& delimiter,
                bool f_opened
            );

            void processStateReadMeta(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter
            );
            
            void processStateReadMeas(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter,
                std::optional<int> step
            );
            
            void processStateReadSummary(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter
            );

            void processStateCompleted(bool& finished);

            void processStateErr(bool& finished);
            

            [[nodiscard]] LogData parsFile(
                const std::string& file_path,
                std::optional<int> step,
                const std::string& delimiter
            );

            // Stores the loaded log data
            std::optional<LogData> log_data_;

    };
} // namespace rrm
