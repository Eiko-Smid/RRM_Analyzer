#pragma once

#include "rrm/log_data.hpp"

#include <cstddef>
#include <optional>
#include <string>

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
            */
            void load(const std::string& file_path, int step = 5);

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
        [[nodiscard]] static LogData parsFile(
            const std::string& file_path,
            int step
        );

        // Stores the loaded log data
        std::optional<LogData> log_data_;

    };
} // namespace rrm