#include <rrm/string_helpers.hpp>


namespace string_helpers
{
    std::string trimTxt(
        const std::string& txt,
        const std::string& chars
    )
    {
        // Find the first character that should not be trimmed.
        const auto first_idx = txt.find_first_not_of(chars);

        // Return an empty string if every character should be trimmed.
        if (first_idx == std::string::npos)
        {
            return {};
        }

        // Find the last character that should not be trimmed.
        const auto last_idx = txt.find_last_not_of(chars);

        return txt.substr(first_idx, last_idx - first_idx + 1);
    }


    std::string lowercaseTxt(std::string txt)
    {
        for (char& c : txt)
        {
            c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(c))
            );
        }

        return txt;
    }


    std::vector<std::string> splitTxt(
        const std::string& txt,
        const std::string& separator
    )
    {
        std::vector<std::string> fields;
        std::size_t start = 0;

        // Ensure valid separator
        if (separator.empty())
            throw std::invalid_argument("Separator must no be empty!");

        // Split text
        while (true)
        {
            // Get first idx of separator 
            const auto separator_idx = txt.find(separator, start);
            
            // Estimate field length
            const auto field_len = (separator_idx == std::string::npos)
                ? std::string::npos 
                : separator_idx - start;
            
            // Extract and append substring to field 
            const std::string field = trimTxt(txt.substr(start, field_len));

            fields.push_back(field);

            if (separator_idx == std::string::npos)
            {
                break;
            }

            // Update start position for next search
            start = separator_idx + separator.size();
        }

        return fields;
    }


    bool isEmptyField(const std::vector<std::string>& fields)
    {
        for (const std::string& field : fields)
        {
            if(!field.empty())
            {
                return false;
            }
        }
        
        return true;
    }


    std::size_t findString(
        const std::vector<std::string>& fields,
        const std::string& str
    )
    {
        for (std::size_t i = 0; i < fields.size(); ++i)
        {
            if (lowercaseTxt(fields[i]) == str)
            {
                return i;
            }
        }

        throw std::runtime_error(
            "Required string" + str + " has not been found inside given fields"
        );
    }


    // Convert a string to a signed 64-bit integer.
    std::int64_t stringToInt(
        const std::string& str,
        const std::string& name,
        std::size_t line_number
    )
    {
        try
        {
            std::size_t consumed = 0;

            // Convert text to long long value
            const auto value = std::stoll(str, &consumed);

            // Check if all chars have been consumed and returned value is same size as int64
            if (
                consumed == str.size() &&
                value >= std::numeric_limits<std::int64_t>::min() &&
                value <= std::numeric_limits<std::int64_t>::max()
            )
                return static_cast<std::int64_t>(value);
        }
        catch (const std::exception&)
        {
            // Skipped cause error below should be thrown instead!
        }

        throw std::runtime_error(
            "Invalid '" + name + "' at line " +
            std::to_string(line_number) + ": '" + str + "'"
        );
    }


    double stringToDouble(
        const std::string& str,
        const std::string& name,
        std::size_t line_number
    )
    {
        try
        {
            std::size_t consumed = 0;

            // Convert string to double value
            const auto value = std::stod(str, &consumed);
            
            // Check if all chars have been consumed and if value is finite
            if (consumed == str.size() && std::isfinite(value))
            {
                return value;
            }            
        }
        catch (const std::exception&)
        {
            // Skipped cause error below should be thrown instead!
        }

        throw std::runtime_error(
            "Invalid '" + name + "' at line " + 
            std::to_string(line_number) + ": '" + str + "'"
        );
    }




}
