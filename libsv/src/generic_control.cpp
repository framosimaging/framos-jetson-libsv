#include "generic_control.hpp"
#include <linux/videodev2.h>
#include <sys/ioctl.h>

MenuEntryList GenericControl::GetMenuEntries()
{
    MenuEntryList menuEntries;

    std::transform(menu.begin(), menu.end(), std::back_inserter(menuEntries), 
        [](const ControlMenuEntry &entry) {
            return MenuEntry {
                entry.name.c_str(),
                entry.index
            };
        }
    );

    return menuEntries; 
}

void GenericControl::Update(ControlMenu menu) 
{
    this->menu = menu;
    maxValue = FinalMenuIndex(menu);
}

void GenericControl::AddDependency(GenericControl *dependency)
{
    dependencies.push_back(dependency);
}

bool GenericControl::Set(int64_t val)
{
    auto result = SetValue(val);
    for (const auto &control : dependencies)
        control->Update();
    return result;
}

int32_t GenericControl::FinalMenuIndex(ControlMenu menu) 
{ 
    if (menu.empty())
        return 0;

    return menu.size() - 1;
}