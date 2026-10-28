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
        std::optional<int> step,
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
        << "\nSelected step: " << (data.sel_step ? std::to_string(*data.sel_step) : "all")
        << "\nTotal measurement rows: " << data.n_meas_rows
        << "\nSelected samples: " << data.meas_samples.size()
        << "\nSelected step finished: " << (data.selected_step_finished ? "yes" : "no")
        << '\n';

        // Display parsing status, including a warning for incomplete files.
        const char* status = "Incomplete";
        if (data.parse_status == ParseStatus::Complete)
            status = "Complete";
        else if (data.parse_status == ParseStatus::Error)
            status = "Error";

        std::cout << "Parsing status: " << status << '\n';
        if (!data.parse_message.empty())
            std::cout << "Parsing message: " << data.parse_message << '\n';

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
            << std::setw(10) << "step"
            << std::setw(14) << "time_ms"
            << std::setw(18) << "radial_force"
            << std::setw(18) << "torque"
            << '\n';

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& sample = data.meas_samples[i];

            std::cout
                << std::setw(10) << sample.step
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
        if (state == ParseState::IDLE && f_opened && !parsing_error)
        {
            state = ParseState::READ_METADATA;
        }
    }

    void RrmLogParser::transIdleToErr(bool f_opened)
    {
        if (state == ParseState::IDLE && (!f_opened || parsing_error))
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadMetaToReadMeas()
    {
        if (state == ParseState::READ_METADATA && found_table && !parsing_error)
        {
            state = ParseState::READ_MEASUREMENTS;
        }

    }


    void RrmLogParser::transReadMetaToErr()
    {
        if (state == ParseState::READ_METADATA &&
            (file_state.eof_reached || file_state.read_err || parsing_error))
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadMeasToReadSum()
    {
        // Check if end of meas has been reached
        if (state == ParseState::READ_MEASUREMENTS && meas_separator_found)
        {
            state = ParseState::READ_SUMMARY;
        }
    }


    void RrmLogParser::transReadMeasToErr()
    {   
        // Change to err state if file loading err occurd, eof has been reached before summary or 
        // a measurement row could not be validated by the state method.
        if (state == ParseState::READ_MEASUREMENTS &&
            (file_state.eof_reached || file_state.read_err || parsing_error))
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadSummaryCompleted()
    {
        // Check if end of summary has been reached
        if (state == ParseState::READ_SUMMARY && file_state.eof_reached &&
            log_stopped_found && !file_state.read_err && !parsing_error)
        {
            state = ParseState::COMPLETED;
        }
    }


    void RrmLogParser::transReadSummaryToErr()
    {
        if (state == ParseState::READ_SUMMARY &&
            (file_state.read_err || parsing_error ||
             (file_state.eof_reached && !log_stopped_found)))
        {
            state = ParseState::ERROR;
        }
    }


    // Reads one line per call. This is a helper used by the three
    // reading-state methods, not a transition or a separate FSM state.
    bool RrmLogParser::readNextLine(
        std::istream& input,
        std::size_t& line_number,
        const std::string& delimiter,
        std::vector<std::string>& fields
    )
    {
        // Define line storage
        std::string line;

        // Read line
        if (!std::getline(input, line))
        {
            // EOF is normal at the end of a complete log. A different
            // read failure must be distinguished from that normal EOF.
            file_state.eof_reached = input.eof();
            file_state.read_err = input.bad() || (!input.eof() && input.fail());
            fields.clear();
            return false;
        }

        ++line_number;
        file_state = {};

        // Remove optional UTF-8 BOM from line
        if (line_number == 1 &&
            line.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            // Remove the first 3 UTF-8 bytes
            line.erase(0, 3);
        }

        // Split line into fields
        fields = string_helpers::splitTxt(line, delimiter);
        return true;
    }


    void RrmLogParser::processStateIdle(
        const std::string& file_path,
        std::optional<int> step,
        const std::string& delimiter,
        bool f_opened
    )
    {
        // Reset 
        col_info = {};
        file_state = {};
        found_table = false;

        // reset err information
        parsing_error = false;
        error_message.clear();

        // Reset new parsing flags.
        meas_separator_found = false;
        log_stopped_found = false;
        selected_step_seen = false;

        // Reset result storage inside the initial FSM state.
        result = {};
        result.file_path = file_path;
        result.sel_step = step;

        // Validate configuration and file-opening result in IDLE.
        if (delimiter.empty())
        {
            parsing_error = true;
            error_message = "Delimiter must not be empty";
        }
        else if (!f_opened)
        {
            error_message = "Cannot open log file: " + file_path;
        }
    }


    void RrmLogParser::processStateReadMeta(
        std::istream& input,
        std::size_t& line_number,
        const std::string& delimiter
    )
    {   
        std::vector<std::string> fields;

        try
        {
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                // The header was interrupted before its column names appeared.
                error_message = file_state.eof_reached
                    ? "Unexpected EOF in metadata: measurement header missing"
                    : "File reading error in metadata";
                return;
            }

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
        catch (const std::exception& e)
        {
            // Save the error instead of losing previously parsed metadata.
            parsing_error = true;
            error_message = e.what();
        }
    }


    void RrmLogParser::processStateReadMeas(
        std::istream& input,
        std::size_t& line_number,
        const std::string& delimiter,
        std::optional<int> step
    )
    {
        std::vector<std::string> fields;

        try
        {
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                error_message = file_state.eof_reached
                    ? "Unexpected EOF in measurement table"
                    : "File reading error in measurement table";
                return;
            }

            // A separator ends the measurement table. It is not a sample.
            if (string_helpers::isEmptyField(fields))
            {
                meas_separator_found = true;
                if (step && selected_step_seen)
                    result.selected_step_finished = true;
                return;
            }

            // Validate the presence of the required columns. A missing
            // trailing empty field is acceptable for these RRM log files.
            const std::size_t required_width = std::max({
                col_info.time_idx,
                col_info.step_idx,
                col_info.radial_force_idx,
                col_info.torque_idx
            }) + 1;

            const bool valid_width =
                fields.size() == col_info.exp_n_columns ||
                fields.size() + 1 == col_info.exp_n_columns;

            if (fields.size() < required_width || !valid_width)
            {
                throw std::runtime_error(
                    "Invalid measurement column count at line " +
                    std::to_string(line_number) +
                    ": expected " + std::to_string(col_info.exp_n_columns) +
                    " (or one fewer trailing field), got " +
                    std::to_string(fields.size())
                );
            }

            // Count measurement rows
            ++result.n_meas_rows;

            // Check if measurements belong to step
            const auto row_step = string_helpers::stringToInt(
                fields[col_info.step_idx],
                "step",
                line_number
            );

            // Record if a later step was seen after the selected one.
            if (step && selected_step_seen && row_step != *step)
                result.selected_step_finished = true;

            // Read in data
            if (!step || row_step == *step)
            {
                if (step)
                {
                    selected_step_seen = true;
                    result.selected_step_finished = false;
                }

                MeasurementSample sample;
                sample.step = row_step;

                sample.time_ms = string_helpers::stringToInt(
                    fields[col_info.time_idx],
                    "time",
                    line_number
                );

                sample.radial_force = string_helpers::stringToDouble(
                    fields[col_info.radial_force_idx],
                    "radial load",
                    line_number
                );

                sample.torque = string_helpers::stringToDouble(
                    fields[col_info.torque_idx],
                    "torque",
                    line_number
                );

                // Update result
                result.meas_samples.push_back(sample);
            }
        }
        catch (const std::exception& e)
        {
            // Preserve already loaded samples on malformed lines.
            parsing_error = true;
            error_message = e.what();
        }
    }


    void RrmLogParser::processStateReadSummary(
        std::istream& input,
        std::size_t& line_number,
        const std::string& delimiter
    )
    {
        std::vector<std::string> fields;

        try
        {
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                if (file_state.read_err)
                    error_message = "File reading error in summary";
                else if (!log_stopped_found)
                    error_message = "Summary completion marker 'log stopped:' missing";
                return;
            }

            // The summary includes empty separator lines; ignore them.
            if (string_helpers::isEmptyField(fields))
                return;

            // Detect the final completion marker without finishing early.
            const std::string first_field =
                string_helpers::lowercaseTxt(fields.front());

            if (first_field.find("log stopped:") == 0)
                log_stopped_found = true;

            storeKeyValue(
                fields,
                result.summary,
                result.summary_notes
            );
        }
        catch (const std::exception& e)
        {
            parsing_error = true;
            error_message = e.what();
        }
    }


    void RrmLogParser::processStateCompleted(bool& finished)
    {
        // A complete log must have the final 'log stopped:' marker.
        result.parse_status = ParseStatus::Complete;
        result.parse_message.clear();
        finished = true;
    }


    void RrmLogParser::processStateErr(bool& finished)
    {
        // An unexpected EOF means a truncated file; all other failures
        // are parsing or I/O errors. Keep successfully loaded samples.
        result.parse_status = (file_state.eof_reached &&
                               !file_state.read_err && !parsing_error)
            ? ParseStatus::Incomplete
            : ParseStatus::Error;
        result.parse_message = error_message;
        finished = true;
    }
    
    LogData RrmLogParser::parsFile(
        const std::string& file_path,
        std::optional<int> step,
        const std::string& delimiter
    )
    {
        // Init members
        // init state
        state = ParseState::IDLE;

        // Represents current line number in file
        std::size_t line_number = 0;

        // Open file
        std::ifstream input(file_path);

        // Check if file has been opened
        const bool f_opened = input.is_open();

        // Only terminal states set this flag, so they run inside the FSM.
        bool finished = false;

        while (!finished)
        {
            // Run state machine
            switch (state)
            {
                case ParseState::IDLE:
                {
                    processStateIdle(file_path, step, delimiter, f_opened);
                    transIdleToErr(f_opened);
                    transIdleToReadMeta(f_opened);
                    break;
                }

                case ParseState::READ_METADATA:
                {
                    processStateReadMeta(input, line_number, delimiter);
                    transReadMetaToErr();
                    transReadMetaToReadMeas();
                    break;
                }

                case ParseState::READ_MEASUREMENTS:
                {
                    processStateReadMeas(input, line_number, delimiter, step);
                    transReadMeasToErr();
                    transReadMeasToReadSum();
                    break;
                }   

                case ParseState::READ_SUMMARY:
                {
                    processStateReadSummary(input, line_number, delimiter);
                    transReadSummaryToErr();
                    transReadSummaryCompleted();
                    break;
                }

                case ParseState::COMPLETED:
                {
                    processStateCompleted(finished);
                    break;
                }

                case ParseState::ERROR:
                {
                    processStateErr(finished);
                    break;
                }
            }
        }

        return result;

    }
    

} // namespace rrm
