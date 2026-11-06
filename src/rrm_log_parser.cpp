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
#include <variant>


namespace rrm
{
    // -----------------------------------------------------------------------------------
    // Implement Helper functions
    // -----------------------------------------------------------------------------------    
    
    /**
     * @brief Stores parsed fields as a key-value pair or a standalone note. Ignores empty fields and uses 
     * the first two fields for key-value pairs.
     * @param fields Vector containing the parsed fields of a log file line.
     * @param values Map used to store key-value pairs.
     * @param notes Vector used to store standalone text entries.
     */
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


    /**
     * @brief Prints a section of the log data to the console. Displays standalone notes first, followed 
     * by key-value pairs sorted alphabetically by key.
     * @param title Title displayed above the section.
     * @param values Map containing the key-value pairs to display.
     * @param notes Vector containing standalone text entries to display.
     */
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

        // Output section
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
        const std::vector<ColumnSpec>& column_specs,
        std::optional<int> step,
        const std::string& delimiter,
        const std::string& table_header_indicator
    )
    {
        // Load data.
        log_data_ = parsFile(file_path, column_specs, step, delimiter, table_header_indicator);

        // Reject a selected step that was not found 
        if (step && log_data_->parse_status == ParseStatus::Complete && !selected_step_seen)
        {
            clear();
            throw std::runtime_error(
                "Step " + std::to_string(*step) +
                " not part of file. Please choose another step."
            );
        }
    }


    void RrmLogParser::display_data(
        std::size_t n_rows,
        std::optional<int> decimal_places
    ) const
    {
        // Check if decimal places parameter is valid
        if (decimal_places && *decimal_places < 0)
        {
            throw std::invalid_argument(
                "Number of decimal places must not be negative."
            );
        }

        // Get the data
        const auto& data = getData();

        // Get the number of selected measurement rows from table if columns exist
        const std::size_t n_selected_meas_rows = data.data_table.columns.empty()
            ? 0
            : data.data_table.columns.front().values.size();

        // Print general table information 
        std::cout
            << "File: " << data.file_path
            << "\nSelected step: " << (data.sel_step ? std::to_string(*data.sel_step) : "all")
            << "\nTotal measurement rows: " << data.n_meas_rows
            << "\nSelected measurement rows: " << n_selected_meas_rows
            << '\n';

        // Display parsing status and message
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

        // Estimate actual row count to display
        const std::size_t count = std::min(
            n_rows, n_selected_meas_rows
        );

        const auto& columns = data.data_table.columns;

        // Helper function to convert a CellValue into a printable string. Apply the specified decimal precision only
        // to floating-point values. If no precision is specified, use the precision for doubles.
        const auto cellToString = [decimal_places](const CellValue& cell) -> std::string
        {
            std::ostringstream stream;

            // Configure stream precision of values
            if (decimal_places)
            {
                stream << std::fixed
                    << std::setprecision(*decimal_places);
            }
            else
            {
                stream << std::defaultfloat
                    << std::setprecision(
                        std::numeric_limits<double>::max_digits10
                    );
            }

            // Define the visitor function that put the value into a stream
            const auto visitor = [&stream](const auto& value)
            {
                stream << value;
            };

            // Convert the cell value to it's stored type and uses lambda to output the value as a stream
            // with given precision
            std::visit(visitor, cell);

            return stream.str();
        };

        // Estimate the column width for each column to be printed based on it's max size.
        // (either its the header or the value with most digits)
        std::vector<std::size_t> col_widths;

        // Reserve memory to store the column width values of each column.
        col_widths.reserve(columns.size());

        for (const auto& col : columns)
        {
            // Get header width
            std::size_t width = col.spec.name.size();

            // Check the width of every value to be printed
            for (std::size_t i = 0; i < count; ++i)
            {
                // Find the max value -> width to be used
                width = std::max(
                    width,
                    cellToString(col.values[i]).size()
                );
            }

            // Add two spaces between columns
            col_widths.push_back(width + 2);
        }

        // Display number of measurement samples
        std::cout << "\nMeasurement samples (first "
                << count << "):\n";

        // Print column names (header)
        for (std::size_t j = 0; j < columns.size(); ++j)
        {
            std::cout
                << std::left
                << std::setw(static_cast<int>(col_widths[j]))
                << columns[j].spec.name;
        }

        std::cout << '\n';

        // Print rows
        for (std::size_t i = 0; i < count; ++i)
        {
            for (std::size_t j = 0; j < columns.size(); ++j)
            {
                // Format and print the value using the calculated column width
                std::cout
                    << std::left
                    << std::setw(static_cast<int>(col_widths[j]))
                    << cellToString(columns[j].values[i]);
            }

            std::cout << '\n';
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
        // Returns reference to log data if existing, throws error else.
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
        // Change to err state if file loading err occurred, eof has been reached before summary or 
        // a measurement row could not be validated by the state method.
        if (state == ParseState::READ_MEASUREMENTS &&
            (file_state.eof_reached || file_state.read_err || parsing_error))
        {
            state = ParseState::ERROR;
        }
    }


    void RrmLogParser::transReadSummaryCompleted()
    {
        // Check if data has been loaded correctly without errors
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
            // Store states when reading fails for later analysis. 
            file_state.eof_reached = input.eof();
            file_state.read_err = input.bad() || (!input.eof() && input.fail());
            fields.clear();
            return false;
        }

        ++line_number;
        file_state = {};

        // Remove optional UTF-8 BOM from line.
        if (line_number == 1 &&
            line.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            // Remove the first 3 UTF-8 bytes.
            line.erase(0, 3);
        }

        // Split line into fields.
        fields = string_helpers::splitTxt(line, delimiter);
        return true;
    }


    void RrmLogParser::processStateIdle(
        const std::string& file_path,
        std::optional<int> step,
        const std::string& delimiter,
        const std::string& table_header_indicator,
        bool f_opened
    )
    {
        // Reset 
        exp_n_columns={};
        file_state = {};
        found_table = false;

        // reset err information.
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
            error_message = "Delimiter must not be empty!";
        }
        else if (table_header_indicator.empty())
        {
            parsing_error = true;
            error_message = "Table header indicator must no be empty!";
        }
        else if (!f_opened)
        {
            error_message = "Cannot open log file: " + file_path;
        }
    }


    void RrmLogParser::processStateReadMeta(
        std::istream& input,
        const std::vector<ColumnSpec>& column_specs,
        std::size_t& line_number,
        const std::string& delimiter,
        const std::string& table_header_indicator
    )
    {   
        // Define storage for rows.
        std::vector<std::string> fields;

        try
        {
            // Try to read next line.
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                // EOF reached before the measurement table header was found.
                error_message = file_state.eof_reached
                    ? "Unexpected EOF in metadata: measurement header missing"
                    : "File reading error in metadata";
                return;
            }

            // Read desired columns headers.
            // Detect first line of measurement table inside log file.
            if (string_helpers::lowercaseTxt(fields.front()) == table_header_indicator)
            {
                // Add step column if not already existing
                std::vector<ColumnSpec> extd_column_specs = column_specs;
                const bool step_exists = std::any_of(
                    column_specs.begin(),
                    column_specs.end(),
                    [](const ColumnSpec& spec)
                    {
                        return spec.name == "step";
                    }
                );

                if (!step_exists)
                {
                    extd_column_specs.push_back({"step", ValueType::Int64});
                }
                
                // Init column information.
                for (const auto& column_spec : extd_column_specs)
                {
                    // Init column 
                    Column col = {};
                    // Check if column exists
                    const auto idx = string_helpers::findString(fields, column_spec.name);
                    if (!idx)
                    {
                        throw std::runtime_error(
                            "Required column" + column_spec.name + " has not been found inside given fields"
                        );
                    }

                    col.idx = *idx;
                    col.spec = column_spec;

                    // Add new column to data table
                    result.data_table.columns.push_back(col);
                }

                // Store current number of columns -> expected number for reading the actual data.
                exp_n_columns = fields.size();
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
            // Store error info
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
        // Define storage for rows.
        std::vector<std::string> fields;

        try
        {
            // Try to read next line
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                error_message = file_state.eof_reached
                    ? "Unexpected EOF in measurement table"
                    : "File reading error in measurement table";
                return;
            }

            // Check of ";" was found after reading the measurement table. If so next section is summary.
            if (string_helpers::isEmptyField(fields))
            {
                // Indicate separator, separating meas section from summary has been found.
                // -> Meas section read successful
                meas_separator_found = true;
                return;
            }
                    
            // Check if columns is missing 
            if (fields.size() != exp_n_columns)
            {
                throw std::runtime_error(
                    "Invalid measurement column count at line " +
                    std::to_string(line_number) +
                    ": expected " + std::to_string(exp_n_columns) +
                    ", got " + std::to_string(fields.size())
                );
            }

            // Count measurement rows.
            ++result.n_meas_rows;

            // Read the step column value
            // Get step column
            const auto step_column = std::find_if(
                result.data_table.columns.begin(),
                result.data_table.columns.end(),
                [](const Column& col)
                {
                    return col.spec.name == "step";
                }
            );

            if (step_column == result.data_table.columns.end())
            {
                throw std::runtime_error("Step column not found!");
            }

            // Get idx of step column
            const std::size_t step_idx = step_column->idx;

            // Read step col value
            const auto step_number = string_helpers::stringToInt(
                fields[step_idx],
                "step",
                line_number
            );

            // Read every row when no step is selected. Otherwise, only read rows
            // belonging to the selected step.
            if (!step || step_number == *step)
            {
                // Record that the selected step was found.
                if (step)
                    selected_step_seen = true;

                // Convert all requested values first -> For safety if convert fails we dont end up with columns of 
                // different size
                std::vector<CellValue> converted_row;
                
                // Convert the values and store in volatile memory 
                for (const auto& col : result.data_table.columns)
                {
                    converted_row.push_back(
                        convertValue(
                            fields[col.idx],
                            col.spec.type,
                            col.spec.name,
                            line_number
                        )
                    );
                }

                // Store the converted values
                for (std::size_t i = 0; i < converted_row.size(); ++i)
                {
                    result.data_table.columns[i].values.push_back(
                        std::move(converted_row[i])
                    );
                }
            }

        }
        catch (const std::exception& e)
        {
            // Store error info
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
        // Define storage for rows.
        std::vector<std::string> fields;

        try
        {
            // Try to read next line.
            if (!readNextLine(input, line_number, delimiter, fields))
            {
                if (file_state.read_err)
                    error_message = "File reading error in summary";
                else if (!log_stopped_found)
                    error_message = "Summary completion marker 'log stopped:' missing";
                return;
            }

            // Ignore empty summary lines.
            if (string_helpers::isEmptyField(fields))
                return;
            
            // Check if log stopped text has been found -> end reached.
            const std::string first_field = string_helpers::lowercaseTxt(fields.front());
            if (first_field.find("log stopped:") == 0)
                log_stopped_found = true;

            // Store summary section in key-value pairs
            storeKeyValue(
                fields,
                result.summary,
                result.summary_notes
            );
        }
        catch (const std::exception& e)
        {
            // Store error info
            parsing_error = true;
            error_message = e.what();
        }
    }


    void RrmLogParser::processStateCompleted(bool& finished)
    {
        // Mark the log read as a success.
        result.parse_status = ParseStatus::Complete;
        result.parse_message.clear();
        finished = true;
    }


    void RrmLogParser::processStateErr(bool& finished)
    {
        // Store the information that the reading failed.
        result.parse_status = (file_state.eof_reached &&
                               !file_state.read_err && !parsing_error)
            ? ParseStatus::Incomplete
            : ParseStatus::Error;

        // Store the loaded data anyway
        result.parse_message = error_message;
        finished = true;
    }

    
    CellValue RrmLogParser::convertValue(
        const std::string& value,
        ValueType type,
        const std::string& col_name,
        std::size_t line_number
    )
    {

        switch (type)
        {
            case ValueType::Int64:
                return string_helpers::stringToInt(
                    value,
                    col_name,
                    line_number
                );
            
            case ValueType::Double:
                return string_helpers::stringToDouble(
                    value,
                    col_name,
                    line_number
                );

            case ValueType::String:
                return value;
        }

        // Raise error if no case occurs
        throw std::logic_error(
            "Unsupported column type for column " + col_name +
            " in row " + std::to_string(line_number) + ".\nType is " + 
            std::to_string(static_cast<int>(type))
        );
    }
    

    LogData RrmLogParser::parsFile(
        const std::string& file_path,
        const std::vector<ColumnSpec>& column_specs,
        std::optional<int> step,
        const std::string& delimiter,
        const std::string& table_header_indicator
    )
    {
        // init state
        state = ParseState::IDLE;
        // Represents current line number in file
        std::size_t line_number = 0;
        // Open file
        std::ifstream input(file_path);
        // Check if file has been opened
        const bool f_opened = input.is_open();
        // Store information if loading has been finished
        bool finished = false;

        while (!finished)
        {
            // Run state machine
            switch (state)
            {
                case ParseState::IDLE:
                {
                    processStateIdle(file_path, step, delimiter, table_header_indicator, f_opened);
                    transIdleToErr(f_opened);
                    transIdleToReadMeta(f_opened);
                    break;
                }

                case ParseState::READ_METADATA:
                {
                    processStateReadMeta(input, column_specs, line_number, delimiter, table_header_indicator);
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

        return std::move(result);

    }
    

} // namespace rrm
