#include <rrm/rrm_log_parser.hpp>

#include <rrm/string_helpers.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>


namespace rrm
{
    // -----------------------------------------------------------------------------------
    // Implement Helper functions
    // -----------------------------------------------------------------------------------    
    
    void storeKeyValue(
        const std::vector<std::string>& fields,
        std::unordered_map<std::string, std::string>& values,
        std::vector<std::string>& notes
    )
    {
        if (string_helpers::isEmptyField(fields))
        {
            return;
        }

        // Store elements with at least one key, value
        if (fields.size() >= 2 &&
            !fields[0].empty() && !fields[1].empty()
        )
        {
            // Use first filed as key and second as value (station:;1;) -> station: 1
            values[fields[0]] = fields[1];
        }

        else if (!fields[0].empty())
        {
            notes.push_back(fields[0]);
        }
    }


    void printSection(
        const char* title,
        const std::unordered_map<std::string, std::string>& values,
        const std::vector<std::string>& notes
    )
    {
        std::cout << "\n" << title << ":\n";

        for (const auto& note : notes)
            std::cout << "  " << note << '\n';

        // Sort keys for consistent terminal output.
        std::vector<std::pair<std::string, std::string>> sorted(
            values.begin(), values.end()
        );

        std::sort(sorted.begin(), sorted.end());

        for (const auto& entry : sorted)
        {
            std::cout << "  " << entry.first
                    << " " << entry.second << '\n';
        }
    }

    // -----------------------------------------------------------------------------------
    // Implement RrmLogParser
    // -----------------------------------------------------------------------------------    
    
    // ------------------------------------------------------------
    // Implement Public methods
    // ------------------------------------------------------------

    void RrmLogParser::load(
        const std::string& file_path,
        int step
    )
    {
        log_data_ = parsFile(file_path, step);
    }


    void RrmLogParser::display_data(std::size_t n_rows) const
    {
        const auto& data = getData();

        std::cout
        << "File: " << data.file_path
        << "\nSelected step: " << data.sel_step
        << "\nTotal measurement rows: " << data.n_meas_table
        << "\nSelected samples: " << data.meas_samples.size()
        << '\n';

        // Display metadata.
        printSection(
            "Metadata",
            data.metadata,
            data.metadata_notes
        );

        // Display selected measurement samples.
        const auto count = std::min(
            n_rows, data.meas_samples.size()
        );

        std::cout
            << "\nMeasurement samples (first "
            << count << "):\n"
            << std::setw(14) << "time_ms"
            << std::setw(18) << "radial_force"
            << std::setw(18) << "torque"
            << '\n';

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& sample = data.meas_samples[i];

            std::cout
                << std::setw(14) << sample.time_ms
                << std::setw(18) << sample.radial_force
                << std::setw(18) << sample.torque
                << '\n';
        }

        // Display summary.
        printSection(
            "Summary",
            data.summary,
            data.summary_notes
        );

    }

    const LogData& RrmLogParser::getData() const
    {
        if (!log_data_)
        {
            throw std::logic_error(
                "No log file loaded. Call load() first."
            );
        }

        return *log_data_;

    }

    bool RrmLogParser::hasLoaded() const noexcept
    {
        return log_data_.has_value();
    }
    

    void RrmLogParser::clear() noexcept
    {
        log_data_.reset();
    }

    // ------------------------------------------------------------
    // Implement Private methods
    // ------------------------------------------------------------
    
    LogData RrmLogParser::parsFile(
        const std::string& file_path,
        int step
    )
    {
        // Open file
        std::ifstream input(file_path);

        // Trow error if can't be opened
        if (!input.is_open())
        {
            throw std::runtime_error(
                "Cannot open log file: " + file_path
            );
        }

        // Create result storage
        LogData result;
        result.file_path = file_path;
        result.sel_step = step;

        enum class Section
        {
            Metadata,
            Measurements,
            Summary
        };

        // Change state to Metadata -> load metadata
        Section section = Section::Metadata;

        // Define idx vars for each element to load
        std::size_t time_idx = 0;
        std::size_t step_idx = 0;
        std::size_t radial_force_idx = 0;
        std::size_t torque_idx = 0;
        std::size_t exp_n_columns = 0;

        bool found_table = false;
        std::size_t line_number = 0;

        std::string line;

        while (std::getline(input, line))
        {
            ++line_number;

            // Remove optional UTF-8 BOM 
            if (line_number == 1 &&
                line.compare(0, 3, "\xEF\xBB\xBF") == 0
            )
            {
                // Remove the first 3 UTF-8 bytes
                line.erase(0, 3);
            }
            
            // Split every element in line separated by ";" into one string and add to vector
            const std::vector<std::string> fields = string_helpers::splitTxt(line, ";");
            
            // 
            if (string_helpers::isEmptyField(fields))
            {
                if (section == Section::Measurements)
                {
                    section = Section::Summary;
                }

                continue;
            }

            // Metadata handling and measurement section preparation
            if (section == Section::Metadata)
            {
                if (string_helpers::lowercaseTxt(fields.front()) == "count")
                {
                    // Estimate column index for the columns we wanne extract
                    time_idx = string_helpers::findString(fields, "time");
                    step_idx = string_helpers::findString(fields, "step");
                    radial_force_idx = string_helpers::findString(fields, "radial load");
                    torque_idx = string_helpers::findString(fields, "torque");

                    // Store n columns 
                    exp_n_columns = fields.size();

                    // Enter new state measurements
                    section = Section::Measurements;
                    found_table = true;

                }
                
                // If we are still in section Metadata
                else
                {
                    // Store metadata as key value pairs
                    storeKeyValue(
                        fields,
                        result.metadata,
                        result.metadata_notes
                    );
                }
            }
            
            // Handling of measurement table
            else if (section == Section::Measurements)
            {   
                // Check if a least the needed columns exist
                if (fields.size() != exp_n_columns)
                {   
                    // throw std::runtime_error(
                    //     "Measurement row has too few columns at line " + std::to_string(line_number) +
                    //     ". Data can't be extracted."                        
                    // );
                    throw std::runtime_error(
                        "Invalid column count at line " +
                        std::to_string(line_number) +
                        ". Initial number of columns: " + std::to_string(exp_n_columns) +
                        "\nCurrent number of columns" + std::to_string(fields.size())
                    );                
                }

                // Count every measurement row.
                ++result.n_meas_table;

                // Check if measurements belong to step -> store or nto -> don't store
                // Get row number from log file 
                const auto row_step = string_helpers::stringToInt(
                    fields[step_idx],
                    "step",
                    line_number
                );
                
                // Skip all steps not equal the one we are searching for
                if (row_step != step)
                {
                    continue;
                }

                // Store measurements
                MeasurementSample sample;

                sample.time_ms = string_helpers::stringToInt(
                    fields[time_idx],
                    "time",
                    line_number
                );

                sample.radial_force = string_helpers::stringToDouble(
                    fields[radial_force_idx],
                    "radial load",
                    line_number
                );

                sample.torque = string_helpers::stringToDouble(
                    fields[torque_idx],
                    "torque",
                    line_number
                );

                result.meas_samples.push_back(sample);
            }

            // Handling of summary section
            else if (section == Section::Summary)
            {
                // Store summary
                storeKeyValue(
                    fields,
                    result.summary,
                    result.summary_notes
                );
            }
            
        }

        if (input.bad())
        {
            throw std::runtime_error(
                "I/O error while reading log file: " + file_path
            );
        }

        // Verify if measurement table has been found
        if (!found_table)
        {
            throw std::runtime_error(
                "Measurement table header not found in: " + file_path
            );
        }

        return result;

    }

    

} // namespace rrm