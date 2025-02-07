#pragma once

#include <string>
#include <vector>

struct ControlMenuEntry
{
    int32_t index;
    std::string name;
};
using ControlMenu = std::vector<ControlMenuEntry>;