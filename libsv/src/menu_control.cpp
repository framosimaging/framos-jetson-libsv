#include "menu_control.hpp"
#include "easylogging++/easylogging++.h"
#include <sys/ioctl.h>
#include <linux/videodev2.h>

void MenuControl::GenerateMenu(uint32_t type)
{
    menu.clear();
    for (int i = minValue; i <= maxValue; ++i) {
        ControlMenuEntry entry = {};
        entry.index = i;
        entry.name = GetMenuEntryName(i, type);
        menu.push_back(entry);
    }
}

std::string MenuControl::GetMenuEntryName(int32_t index, uint32_t type)
{
    v4l2_querymenu qmenu = {};
    qmenu.id = id;
    qmenu.index = index;
    if(::ioctl(fd, VIDIOC_QUERYMENU, &qmenu) != 0) {
        LOG(ERROR) << "Unable to acquire menu entry name";
        return "Unknown";
    }
    
    switch (type) {
        case V4L2_CTRL_TYPE_MENU:
            return std::string((char*)qmenu.name, 32);
        default:
            return std::to_string(qmenu.value);
    }
}