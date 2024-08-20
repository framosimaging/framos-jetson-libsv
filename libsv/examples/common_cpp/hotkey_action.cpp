#include "hotkey_action.hpp"

#include <algorithm>
#include <sstream>

namespace common
{

HotkeyAction::HotkeyAction(std::vector<unsigned char> hotkeys, std::string description, std::function<void()> action)
{
    this->hotkeys = hotkeys;

    std::stringstream descriptionStream;
    bool first = true;
    for (auto const &hotkey : hotkeys) {
        if (!first) {
            descriptionStream << ", ";
        }
        descriptionStream << ParseHotkey(hotkey);
        first = false;
    }
    descriptionStream << " - " << description;
    this->description = descriptionStream.str();

    this->action = action;
}

HotkeyAction::~HotkeyAction()
{

}

std::string HotkeyAction::ParseHotkey(unsigned char hotkey)
{
    switch(hotkey) {
        case SpecialHotkey::ENTER:
        case SpecialHotkey::ENTER_NUM:
            return "ENTER";
        case SpecialHotkey::ESC:
            return "ESC";
        default:
            return std::string(1, hotkey);
    }
}

std::string HotkeyAction::GetDescription() const
{
    return description;
}

bool HotkeyAction::IsValidHotkey(unsigned char hotkey) const
{
    return std::find(hotkeys.begin(), hotkeys.end(), hotkey) != hotkeys.end();
}

void HotkeyAction::Perform() const
{
    this->action();
}
    
}
