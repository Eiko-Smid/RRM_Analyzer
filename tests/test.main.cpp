
#include "rrm/rrm_log_parser.hpp"

#include <exception>
#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr
            << "Usage: rrm_analyzer <log_file> [step]\n";
        return 1;
    }

    try
    {
        rrm::RrmLogParser parser;

        const int step = argc > 2
            ? std::stoi(argv[2])
            : 5;

        parser.load(argv[1], step);
        parser.display_data(10);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
