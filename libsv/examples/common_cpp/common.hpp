#pragma once

#include <iostream>
#include <sstream>
#include <set>
#include <cctype>
#include <algorithm>
#include <limits>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "sv/sv.h"

#include "input.hpp"
#include "resize_options.hpp"
#include "frame_size.hpp"

namespace common
{    
    void SelectPixelFormat(IControl* control)
    {
        MenuEntryList menu = control->GetMenuEntries();
        if (menu.empty()) {
            std::cout << "Pixel format menu empty." << std::endl;
            return;
        }

        std::cout << "Choose a pixel format [0]\n";
        for (const MenuEntry& entry : menu) {
            std::cout << entry.index  << " - " << entry.name << std::endl;
        }

        int32_t index = QueryNumber(0, menu.size() - 1, 0);
        control->Set(index);

        if (control->Get() != index) {
            std::cout << "Failed to set pixel format.\n";
            std::cout << "Current pixel format is:\n";
            std::cout << "ID: " << menu[control->Get()].index << " Name:" << menu[control->Get()].name << std::endl;
        }
    }

    void SelectFrameSize(IControl* frmsizControl)
    {
        MenuEntryList menufrmsiz = frmsizControl->GetMenuEntries();
        if (menufrmsiz.empty()) {
            std::cout << "Frame size menu empty." << std::endl;
            return;
        }

        std::cout << "Choose a frame size [0]\n";
        for (const MenuEntry& entry : menufrmsiz) {
            std::cout << entry.index  << " - " << entry.name << std::endl;
        }

        int32_t index = QueryNumber(0, menufrmsiz.size() - 1, 0);
        frmsizControl->Set(index);

        if (frmsizControl->Get() != index) {
            std::cout << "Failed to set frame size.\n";
            std::cout << "Current Frame size is:\n";
            std::cout << "ID: " << menufrmsiz[frmsizControl->Get()].index << " Name:" << menufrmsiz[frmsizControl->Get()].name << std::endl;
        }
    }

    ICamera* SelectCamera(const ICameraList& cameras)
    {
        std::cout << "Choose a camera [0]\n";
        int i = 0;
        for (ICamera *camera : cameras) {
            std::cout << i << " - " << camera->GetName() << " (" << camera->GetDriverName() << ")" << std::endl;
            i += 1;
        }

        int32_t index = QueryNumber(0, cameras.size() - 1, 0);

        return cameras[index];
    }

    int32_t SelectValue(std::string name, int32_t minValue, int32_t maxValue, int32_t defaultValue)
    {
        std::cout << "Enter " << name << " [" << defaultValue << "]" << std::endl;
        std::cout << "Min: " << minValue << std::endl;
        std::cout << "Max: " << maxValue << std::endl;

        return QueryNumber(minValue, maxValue, defaultValue, true);
    }
    
    bool SelectEnable(std::string name, bool defaultValue)
    {
        std::string defaultValueText;
        if (defaultValue == true) {
            defaultValueText = "true";
        } else {
            defaultValueText = "false";
        }

        std::cout << "Enable " << name << " [" << defaultValueText << "]" << std::endl;
        std::cout << "false" << std::endl;
        std::cout << "true" << std::endl;
        return QueryBool(defaultValue);
    }

    int32_t SelectFromMenu(std::string name, std::vector<std::string> menu, uint32_t defaultIndex = 0)
    {
        std::cout << "Select " << name << " [" << defaultIndex << "]" << std::endl;
        
        for (uint32_t i = 0; i < menu.size(); ++i) {
            std::cout << i << " - " << menu[i] << std::endl;
        }

        return QueryNumber(0, menu.size() - 1, defaultIndex);
    }

    std::vector<std::string> GenerateCameraMenu(const ICameraList& cameras, bool appendDone = false)
    {
        std::vector<std::string> menu;

        std::transform(cameras.begin(), cameras.end(), std::back_inserter(menu), 
            [] (ICamera *camera) { 
                return std::string(camera->GetName()) + " (" + camera->GetDriverName() + ")"; 
            }
        );

        if (appendDone) {
            menu.push_back("Done");
        } 

        return menu;
    }

    ICameraList SelectMultipleCameras(const ICameraList& cameras)
    {        
        ICameraList menu = cameras;

        ICameraList selectedCameras;

        uint32_t selected = common::SelectFromMenu("camera", GenerateCameraMenu(menu));
        selectedCameras.push_back(menu[selected]);
        menu.erase(menu.begin() + selected);

        std::move(menu.begin() + selected, menu.begin() + selected, selectedCameras.end());

        while (!menu.empty()) {
            
            selected = common::SelectFromMenu("camera", GenerateCameraMenu(menu, true));
            
            if (selected == menu.size())
                break;
            
            selectedCameras.push_back(menu[selected]);
            menu.erase(menu.begin() + selected);
        }

        return selectedCameras;
    }

    void WaitForEnter(std::string message)
    {
        std::cout << message << std::endl;
        std::cin.get();
    }

    void ExitOnEnter()
    {
        WaitForEnter("Press ENTER to exit...");
    }
}
