#pragma once

#include "int32_control.hpp"

class MenuControl final : public Int32Control
{
    public:
        MenuControl(int fd, v4l2_queryctrl control) : Int32Control(fd, control)
        { 
            GenerateMenu(control.type);
        }

    private:
        bool isMenuGenerated = false;
        std::string GetMenuEntryName(int32_t index, uint32_t type);
        void GenerateMenu(uint32_t type);
};