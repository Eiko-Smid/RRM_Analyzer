#pragma once

#include "rrm/log_data.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <iosfwd>

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
             * @param delimiter The delimiter to split the logfile lines into chunks.
            */
            void load(
                const std::string& file_path,
                int step = 5,
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
            ParseState state;
            FileState file_state;
            ColInfo col_info;

            // Indicates if measurement table has been found in log file
            bool found_table;

            // Stores data while parsing the log file
            LogData result;

            bool parsing_error{false};
            std::string error_message;

            // Define transitions
            void transIdleToReadMeta(bool f_opened);

            void transIdleToErr(bool f_opened);
            
            void transReadMetaToReadMeas();

            void transReadMetaToErr();

            void transReadMeasToReadSum(const std::vector<std::string> fields);

            void transReadMeasToErr(const std::vector<std::string> fields);

            void transReadSummaryCompleted(const std::vector<std::string>& fields);

            void transReadSummaryToErr();
            
            void processStateIdle();

            void processStateReadMeta(const std::vector<std::string> fields);
            
            void processStateReadMeas(
                const std::vector<std::string> fields,
                std::size_t line_number,
                int step
            );
            
            void processStateReadSummary(const std::vector<std::string> fields);

            void processStateCompleted();

            void processStateErr();
            
            bool readNextLine(
                std::istream& input,
                std::size_t& line_number,
                const std::string& delimiter,
                std::vector<std::string>& fields
            );

            [[nodiscard]] LogData parsFile(
                const std::string& file_path,
                int step,
                const std::string& delimiter
            );

            // Stores the loaded log data
            std::optional<LogData> log_data_;

    };
} // namespace rrm
