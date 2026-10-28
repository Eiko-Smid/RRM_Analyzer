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
        int step,
        const std::string& delimiter
    )
    {
        log_data_ = parsFile(file_path, step, delimiter);
    }


    void RrmLogParser::display_data(std::size_t n_rows) const
    {
        const auto& data = getData();

        std::cout
        << "File: " << data.file_path
        << "\nSelected step: " << data.sel_step
        << "\nTotal measurement rows: " << data.n_meas_rows
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

    void RrmLogParser::transIdleToReadMeta(bool f_opened)
    {
        if (f_opened)
        {
            state = ParseState::READ_METADATA;
        }
    }

    void RrmLogParser::transIdleToErr(bool f_opened)
    {
        if (!f_opened)
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadMetaToReadMeas()
    {
        if (found_table)
        {
            state = ParseState::READ_MEASUREMENTS;
        }

    }


    void RrmLogParser::transReadMetaToErr()
    {
        if (file_state.eof_reached ||
            file_state.read_err ||
            parsing_error)
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadMeasToReadSum(const std::vector<std::string> fields)
    {
        // Check if end of meas has been reached
        if (string_helpers::isEmptyField(fields))
        {
            state = ParseState::COMPLETED;
        }
    }


    void RrmLogParser::transReadMeasToErr(const std::vector<std::string> fields)
    {   
        // Change to err state if file loading err occurd, eof has been reached before summary or 
        // the number of columns in the iteration doesn't match the one from the first read
        if (
            file_state.eof_reached ||
            file_state.read_err ||
            fields.size() != col_info.exp_n_columns

        )
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadSummaryCompleted(const std::vector<std::string>& fields)
    {
        // Check if end of summary has been reached
        if (string_helpers::isEmptyField(fields))
        {
            state = ParseState::COMPLETED;
        }
    }


    void RrmLogParser::transReadSummaryToErr()
    {
        if(file_state.eof_reached || file_state.read_err)
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::processStateIdle()
    {
        // Reset 
        col_info = {};
        file_state = {};
        found_table = false;

        // reset err information
        parsing_error = false;
        error_message.clear();

    }


    void RrmLogParser::processStateReadMeta(const std::vector<std::string> fields)
    {
        if (state == ParseState::READ_METADATA)
        {
            if (string_helpers::lowercaseTxt(fields.front()) == "count")
            {
                // Init column information
                col_info.time_idx = string_helpers::findString(fields, "time");
                col_info.step_idx = string_helpers::findString(fields, "step");
                col_info.radial_force_idx = string_helpers::findString(fields, "radial load");
                col_info.torque_idx = string_helpers::findString(fields, "torque");
                col_info.exp_n_columns = fields.size();            
                found_table = true;
            }
            else
            {
                storeKeyValue(
                    fields,
                    result.metadata,
                    result.metadata_notes
                );
            }
        }
        
    }


    void RrmLogParser::processStateReadMeas(const std::vector<std::string> fields, std::size_t line_number, int step)
    {
        // Count measurement rows
        ++result.n_meas_rows;

        // Check if measurements belong to step
        const auto row_step = string_helpers::stringToInt(
            fields[col_info.step_idx],
            "step",
            line_number
        );

        // Read in data
        if (row_step == step)
        {
            MeasurementSample sample;

            sample.time_ms = string_helpers::stringToInt(
                fields[col_info.time_idx],
                "time",
                line_number
            );

            sample.time_ms = string_helpers::stringToInt(
                fields[col_info.radial_force_idx],
                "radial load",
                line_number
            );

            sample.time_ms = string_helpers::stringToInt(
                fields[col_info.torque_idx],
                "torque",
                line_number
            );

            // Update result
            result.meas_samples.push_back(sample);
        }
    }


    void RrmLogParser::processStateReadSummary(const std::vector<std::string> fields)
    {
        storeKeyValue(
            fields,
            result.summary,
            result.summary_notes
        );
    }


    void RrmLogParser::processStateCompleted()
    {
    }


    void RrmLogParser::processStateErr()
    {
    }

 
    bool RrmLogParser::readNextLine(
        std::istream& input,
        std::size_t& line_number,
        const std::string& delimiter,
        std::vector<std::string>& fields
    )
    {
        std::string line;

        // Check if line could be read
        if (!std::getline(input, line))
        {
            file_state.eof_reached = input.eof();
            file_state.read_err = !input.eof();
            fields.clear();
            return false;
        }

        ++line_number;
        file_state = {};

        // Remove UTF-8 BOM from first line
        if (line_number == 1 &&
            line.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            line.erase(0, 3);
        }

        // Split line into fields
        fields = string_helpers::splitTxt(line, delimiter);

        return true;
    }
    
    
    LogData RrmLogParser::parsFile(
            const std::string& file_path,
            int step,
            const std::string& delimiter
    )
    {
        // Init members
        // init state
        state = ParseState::IDLE;
        // init storage for data
        log_data_.reset();

        // Reset result storage
        result = {};
        result.file_path = file_path;
        // Defines the step in the log file to load
        result.sel_step = step;
        
        // Represents current line number in file
        std::size_t line_number = 0;
        // Storage for each line
        std::string line;

        // Open file
        std::ifstream input(file_path);

        // Check if file has been opened
        bool f_opened = input.is_open();

        const bool line_read = false;

        const std::vector<std::string> fields;
        
        
        while (state!=ParseState::COMPLETED && state!=ParseState::ERROR)
        {
            // Read line
            if (state != ParseState::IDLE)
            {
                const bool line_read = static_cast<bool>(std::getline(input, line));

                // Implement eof and read err checks
                file_state.eof_reached = !line_read && input.eof();
                file_state.read_err = !line_read && !input.eof();

                // Prepare line information
                if (line_read)
                {
                    ++line_number;
                }

                // Remove optional UTF-8 BOM from line
                if (line_number == 1 &&
                    line.compare(0, 3, "\xEF\xBB\xBF") == 0
                )
                {
                    // Remove the first 3 UTF-8 bytes
                    line.erase(0, 3);
                }

                // Split line into fields
                const std::vector<std::string> fields = string_helpers::splitTxt(line, delimiter);
            }            


            // Run state machine
            switch (state)
            {
                case ParseState::IDLE:
                {
                    transIdleToErr(f_opened);
                    processStateIdle();
                    transIdleToReadMeta(f_opened);
                    break;
                }

                case ParseState::READ_METADATA:
                {
                    transReadMetaToErr();
                    processStateReadMeta(fields);
                    transReadMetaToReadMeas();
                    break;
                }

                case ParseState::READ_MEASUREMENTS:
                {
                    transReadMeasToErr(fields);
                    transReadMeasToReadSum(fields);
                    processStateReadMeas(
                        fields,
                        line_number,
                        step
                    );
                    break;
                }   

                case ParseState::READ_SUMMARY:
                {
                    transReadSummaryToErr();
                    processStateReadSummary(fields);
                    transReadSummaryCompleted(fields);
                    break;
                }

                case ParseState::COMPLETED:
                {
                    processStateCompleted();
                    break;
                }

                case ParseState::ERROR:
                {
                    processStateErr();
                    break;
                }
            }

        }

        return result;

    }
    

} // namespace rrm
