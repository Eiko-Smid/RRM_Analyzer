
#include "rrm/rrm_log_parser.hpp"

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
namespace
{

// Remove whitespace from both ends of a string.
std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return {};

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

// Convert to lowercase for column-name comparisons.
std::string lowercase(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        }
    );

    return value;
}

// Split a semicolon-separated line into fields.
std::vector<std::string> splitLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::size_t start = 0;

    while (true)
    {
        const auto separator = line.find(';', start);

        fields.push_back(trim(line.substr(
            start,
            separator == std::string::npos
                ? std::string::npos
                : separator - start
        )));

        if (separator == std::string::npos)
            break;

        start = separator + 1;
    }

    return fields;
}

// Check whether all fields are empty.
bool isSeparator(const std::vector<std::string>& fields)
{
    return std::all_of(
        fields.begin(), fields.end(),
        [](const std::string& field)
        {
            return field.empty();
        }
    );
}

// Find a required column by its header name.
std::size_t columnIndex(
    const std::vector<std::string>& names,
    const std::string& required
)
{
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (lowercase(names[i]) == required)
            return i;
    }

    throw std::runtime_error(
        "Required measurement column not found: " + required
    );
}

// Convert a field to a signed 64-bit integer.
std::int64_t readInteger(
    const std::string& text,
    const std::string& name,
    std::size_t line_number
)
{
    try
    {
        std::size_t consumed = 0;
        const auto value = std::stoll(text, &consumed);

        if (consumed == text.size() && !text.empty())
            return value;
    }
    catch (const std::exception&)
    {
        // Generate one descriptive error below.
    }

    throw std::runtime_error(
        "Invalid '" + name + "' at line " +
        std::to_string(line_number) + ": '" + text + "'"
    );
}

// Convert a field to double using decimal-point notation.
double readDouble(
    const std::string& text,
    const std::string& name,
    std::size_t line_number
)
{
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());

    double value = 0.0;
    stream >> value;

    if (!stream || !std::isfinite(value))
    {
        throw std::runtime_error(
            "Invalid '" + name + "' at line " +
            std::to_string(line_number) + ": '" + text + "'"
        );
    }

    stream >> std::ws;

    if (!stream.eof())
    {
        throw std::runtime_error(
            "Invalid '" + name + "' at line " +
            std::to_string(line_number) + ": '" + text + "'"
        );
    }

    return value;
}

// Store header or summary entries.
// Key-value pairs go into the map; standalone lines into notes.
void storeKeyValue(
    const std::vector<std::string>& fields,
    std::unordered_map<std::string, std::string>& values,
    std::vector<std::string>& notes
)
{
    if (isSeparator(fields))
        return;

    if (fields.size() >= 2 &&
        !fields[0].empty() && !fields[1].empty())
    {
        values[fields[0]] = fields[1];
    }
    else if (!fields[0].empty())
    {
        notes.push_back(fields[0]);
    }
}

// Print metadata or summary in the terminal.
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

} // anonymous namespace


// ============================================================
// Public methods
// ============================================================

void RrmLogParser::load(
    const std::string& file_path,
    int step
)
{
    // If parsing fails, previously loaded data is preserved.
    log_data_ = parsFile(file_path, step);
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


// ============================================================
// Private methods
// ============================================================

LogData RrmLogParser::parsFile(
    const std::string& file_path,
    int step
)
{
    std::ifstream input(file_path);

    if (!input.is_open())
    {
        throw std::runtime_error(
            "Cannot open log file: " + file_path
        );
    }

    LogData result;
    result.file_path = file_path;
    result.sel_step = step;

    enum class Section
    {
        Metadata,
        Measurements,
        Summary
    };

    Section section = Section::Metadata;

    std::size_t time_idx = 0;
    std::size_t step_idx = 0;
    std::size_t radial_idx = 0;
    std::size_t torque_idx = 0;
    std::size_t required_width = 0;

    bool found_table = false;
    std::size_t line_number = 0;

    std::string line;

    while (std::getline(input, line))
    {
        ++line_number;

        // Remove optional UTF-8 BOM from first line.
        if (line_number == 1 &&
            line.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            line.erase(0, 3);
        }

        const auto fields = splitLine(line);

        // An empty separator ends the measurement table.
        if (isSeparator(fields))
        {
            if (section == Section::Measurements)
                section = Section::Summary;

            continue;
        }

        // ----------------------------------------------------
        // Metadata and measurement header
        // ----------------------------------------------------
        if (section == Section::Metadata)
        {
            if (lowercase(fields.front()) == "count")
            {
                time_idx = columnIndex(fields, "time");
                step_idx = columnIndex(fields, "step");
                radial_idx = columnIndex(fields, "radial load");
                torque_idx = columnIndex(fields, "torque");

                required_width = std::max({
                    time_idx,
                    step_idx,
                    radial_idx,
                    torque_idx
                }) + 1;

                section = Section::Measurements;
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

        // ----------------------------------------------------
        // Measurement table
        // ----------------------------------------------------
        else if (section == Section::Measurements)
        {
            if (fields.size() < required_width)
            {
                throw std::runtime_error(
                    "Measurement row has too few columns at line " +
                    std::to_string(line_number)
                );
            }

            // Count every measurement row.
            ++result.n_meas_rows;

            // Check step before parsing numeric measurements.
            const auto row_step = readInteger(
                fields[step_idx], "step", line_number
            );

            if (row_step != step)
                continue;

            MeasurementSample sample;

            sample.time_ms = readInteger(
                fields[time_idx], "time", line_number
            );

            sample.radial_force = readDouble(
                fields[radial_idx], "radial load", line_number
            );

            sample.torque = readDouble(
                fields[torque_idx], "torque", line_number
            );

            result.meas_samples.push_back(sample);
        }

        // ----------------------------------------------------
        // Summary
        // ----------------------------------------------------
        else
        {
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

    if (!found_table)
    {
        throw std::runtime_error(
            "Measurement table header not found in: " + file_path
        );
    }

    return result;
}

} // namespace rrm
