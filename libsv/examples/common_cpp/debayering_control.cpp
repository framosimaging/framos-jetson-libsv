#include "debayering_control.hpp"
#include <stdexcept>

namespace common 
{

DebayeringControl::DebayeringControl()
{
    entries.push_back(MenuEntry{"disabled", 0});
    entries.push_back(MenuEntry{"enabled", 1});
    min = 0;
    max = entries.size() - 1;
    value = min;
}

DebayeringControl::~DebayeringControl()
{

}
            
uint32_t DebayeringControl::GetID()
{
    throw std::invalid_argument("Custom control does not have an ID");
}

const char* DebayeringControl::GetName()
{
    return "Debayering";
}

int64_t DebayeringControl::Get()
{
    return value;
}

bool DebayeringControl::Set(int64_t val) 
{
    if (val < min || val > max) { 
        return false;
    }
    value = val;
    return true;
}

int64_t DebayeringControl::GetMinValue()
{
    return min;
}

int64_t DebayeringControl::GetMaxValue()
{
    return max;
}
            
int64_t DebayeringControl::GetStepValue()
{
    return 1;
}
            
int64_t DebayeringControl::GetDefaultValue()
{
    return min;
}

MenuEntryList DebayeringControl::GetMenuEntries()
{
    return entries;
}

bool DebayeringControl::IsMenu() 
{
    return true;
}

}