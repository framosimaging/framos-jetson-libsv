#include "string_util.hpp"

namespace common
{

std::vector<std::string> SplitString(std::string string, std::string delimiter)
{
    std::vector<std::string> tokens;

    size_t delimiterPosition = string.find(delimiter);
    while(delimiterPosition != std::string::npos){

        std::string token = string.substr(0, delimiterPosition);
        tokens.push_back(token);
        
        string.erase(0, delimiterPosition + delimiter.length());

        delimiterPosition = string.find(delimiter);
    }
    tokens.push_back(string);

    return tokens; 
}

}