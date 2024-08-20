#pragma once

#include <iostream>
#include <set>
#include <algorithm>

namespace common
{

int32_t QueryNumber(int32_t minInclusive, int32_t maxInclusive, int32_t defaultValue, bool clamp = false)
{
    int32_t selectedNumber = defaultValue;
    bool validNumber = false;
    do {
        std::cout << ">";

        std::string selection;
        std::getline(std::cin, selection);

        if (selection == "") {
            return defaultValue;
        }

        validNumber = true;
        try {
            selectedNumber = std::stoi(selection);
            if (clamp) {
                if (selectedNumber < minInclusive) {
                    std::cout << "Clamping to minimum!" << std::endl;
                    selectedNumber = minInclusive;
                }
                else if (selectedNumber > maxInclusive) {
                    std::cout << "Clamping to maximum!" << std::endl;
                    selectedNumber = maxInclusive;
                }
            }
            else if (selectedNumber < minInclusive || selectedNumber > maxInclusive) {
                std::cout << "Number out of bounds!" << std::endl;
                validNumber = false;
            }
        } catch (...) {
            std::cout << "Invalid number!" << std::endl;
            validNumber = false;
        }

    } while(selectedNumber < minInclusive || selectedNumber > maxInclusive || !validNumber);

    std::cout << std::endl;

    return selectedNumber;
}

bool QueryBool(bool defaultValue)
{
    std::set<std::string> trueValues = { "y", "yes", "true", "ok", "1" };
    std::set<std::string> falseValues = { "n", "no", "false", "0" };
    
    do {
        std::string line;
        std::getline(std::cin, line);
        
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        
        if (line == "") {
            return defaultValue;
        } else if (trueValues.find(line) != trueValues.end()) {
            std::cout << std::endl;
            return true;
        } else if (falseValues.find(line) != falseValues.end()) {
            std::cout << std::endl;
            return false;
        } else {
            std::cout << "Invalid input" << std::endl;
        }
    } while(true);
}

}