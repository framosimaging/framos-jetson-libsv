#pragma once

#include <vector>
#include <functional>
#include <string>

namespace common
{
    enum SpecialHotkey { ENTER = 13, ESC = 27, ENTER_NUM = 141 };

    class HotkeyAction
    {
        public:
            HotkeyAction(std::vector<unsigned char> hotkeys, std::string description, std::function<void()> action);
            ~HotkeyAction();
            std::string GetDescription() const;
            bool IsValidHotkey(unsigned char hotkey) const;
            void Perform() const;
        
        private:
            std::string description;
            std::vector<unsigned char> hotkeys;
            std::function<void()> action;
            std::string ParseHotkey(unsigned char hotkey);
    };
}
