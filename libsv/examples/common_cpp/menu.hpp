#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

namespace common
{
    constexpr auto exitMenuKey = "q";

    enum class Selection { Selected, Repeat, Exit };

    template <class TValue>
    class DisplayMenuEntry {
        public:
            explicit DisplayMenuEntry(std::string description, TValue value) 
            { 
                this->description = description;
                this->value = value;
            }
            ~DisplayMenuEntry() { }
            std::string GetDescription() { return description; }
            TValue GetValue() { return value; }
        private:
            std::string description;
            TValue value;
    };

    void ClearTerminal()
    {
        if (std::system("clear") != 0) {
            std::cout << "---------------------------------------------" << std::endl;
        }
    }

    Selection SelectValue(int32_t min, int32_t max, int32_t &index, bool clamp = false)
    {
        std::cout << ">" << std::flush;

        std::string selection;
        std::getline(std::cin, selection);

        if (selection == exitMenuKey || selection.empty()) {
            return Selection::Exit;
        }

        try {
            index = std::stoi(selection);
            if (clamp) {
                if (index < min) {
                    std::cout << "Clamping to minimum!" << std::endl;
                    index = min;
                }
                else if (index > max) {
                    std::cout << "Clamping to maximum!" << std::endl;
                    index = max;
                }
            } else if (index < min || index > max) {
                std::cout << "Number out of bounds!" << std::endl;
                return Selection::Repeat;
            }
        } catch (...) {
            std::cout  << "Invalid number!" << std::endl;
            return Selection::Repeat;
        }

        return Selection::Selected;
    }

    template <class TValue>
    bool GetMenuEntry(std::string prompt, std::vector<DisplayMenuEntry<TValue>> menu, TValue &selected)
    {
        ClearTerminal();

        std::cout << prompt << " [" << exitMenuKey << "]:" << std::endl;

        for (uint32_t i = 0; i < menu.size(); ++i) {
            std::cout << i << " - " << menu[i].GetDescription() << std::endl;
        }

        while (true) {
            int32_t index;
            switch (SelectValue(0, menu.size() - 1, index)) {
            case Selection::Selected:
                selected = menu[index].GetValue();
                std::cout << std::endl;
                return true;
            case Selection::Exit:
                std::cout << std::endl;
                return false;
            case Selection::Repeat:
                break;
            }
        }
    }

    int32_t GetInteger(std::string prompt, int32_t min, int32_t max, int32_t step, int32_t defaultValue, int32_t &selected)
    {
        ClearTerminal();

        std::cout << prompt << " [" << exitMenuKey << "]:" << std::endl;
        std::cout << "Min: " << min << std::endl;
        std::cout << "Max: " << max << std::endl;
        std::cout << "Step: " << std::to_string(step) << std::endl;
        std::cout << "Default: " << std::to_string(defaultValue) << std::endl;

        while (true) {
            switch (SelectValue(min, max, selected, true)) {
            case Selection::Selected:
                std::cout << std::endl;
                return true;
            case Selection::Exit:
                std::cout << std::endl;
                return false;
            case Selection::Repeat:
                break;
            }
        }
    }
}