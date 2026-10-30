#pragma once

#include <string>
// #include <iostream>
// #include <algorithm>
#include <cctype>
#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <cmath>


namespace string_helpers
{   
    /** @brief Removes specified characters from both ends of the given text.
     * @param txt A Reference to the string that will be trimmed.
     * @param chars Characters to remove from the beginning and end of the text.
     * @return The trimmed string.
     */
    std::string trimTxt(
        const std::string& txt,
        const std::string& chars = " \t\r\n"
    );
    
    /** @brief Get's a text and returns the text in lowercase letters.
     * @param txt The string that should be returned in lowercase letters.
     * @return The given text in lower case
     */
    std::string lowercaseTxt(std::string txt);

    /** @brief Get's a text and a separator and splits the text into substrings.
     * @param txt The text to split up
     * @param separator The string that separates the txt elements
     * @return A vector containing the split up elements of the text
    */
    std::vector<std::string> splitTxt(
        const std::string& txt,
        const std::string& separator = ";"
    );

    /** @brief Checks if the given vector contains a string that is not empty
     * @param fields The vector containing the individual fields
     * @return True if all elements are empty, else false.
     */
    bool isEmptyField(const std::vector<std::string>& fields);


    /** @brief Searches for the given string inside the filed vector. Returns the position of the string if found,
     * else it throws an error. 
     * @param fields A vector containing the strings
     * @param str The string to search for in the vector
     */
    std::size_t findString(
        const std::vector<std::string>& fields,
        const std::string& str
    );

    /**
     * @brief Converts a given string into a int 64 number. If the string contains more than a number than an error
     * msg with given name and line number will be printed.
     * @param string The string that should be converted.
     * @param name The name that should be printed when error msg will be thrown.
     * @param line_number The line number that should be printed.
     * @return std::int64_t 
     */
    std::int64_t stringToInt(
        const std::string& str,
        const std::string& name,
        std::size_t line_number
    );


    double stringToDouble(
        const std::string& str,
        const std::string& name,
        std::size_t line_number
    );
    
} // namespace txt_manipulation
