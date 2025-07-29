#include "camera_configurator.hpp"
#include "menu.hpp"
#include "debayering_control.hpp"
#include "resize_options.hpp"
#include "frame_size.hpp"
#include "string_util.hpp"

#include <iostream>
#include <algorithm>

namespace common 
{

CameraConfigurator::CameraConfigurator(ICameraList cameras) : cameras(cameras)
{

}

CameraConfigurator::~CameraConfigurator()
{

}

ConfiguredCameras CameraConfigurator::Configure()
{
    ConfiguredCameras configuredCameras;

    std::transform(cameras.begin(), cameras.end(), std::back_inserter(configuredCameras),
        [](ICamera *camera) { return ConfiguredCamera {camera, false, { true, VgaFrameSize().GetWidth(), VgaFrameSize().GetHeight() }}; }
    );

    int32_t index;
    while (GetConfigurationMenuEntry(index))
        ConfigureControls(configuredCameras[index]);

    return configuredCameras;
}

void CameraConfigurator::ConfigureControls(ConfiguredCamera &configuredCamera)
{
    debayeringControl.Set(configuredCamera.debayering);

    FrameSize configuredResize(configuredCamera.resizeOptions.width, configuredCamera.resizeOptions.height);
    if (configuredCamera.resizeOptions.enable == false)
        resizeControl.Set(ResizeValue::None);
    else if (configuredResize == FullHdFrameSize())
        resizeControl.Set(ResizeValue::FullHd);
    else if (configuredResize == HdFrameSize())
        resizeControl.Set(ResizeValue::Hd);
    else if (configuredResize == VgaFrameSize())
        resizeControl.Set(ResizeValue::Vga);
    else
        resizeControl.Set(ResizeValue::Vga);

    IControl *control = nullptr;
    while (GetControlEntry(configuredCamera.camera, control))
        ConfigureControl(control, configuredCamera.camera->GetName());

    configuredCamera.debayering = debayeringControl.Get();
    switch(resizeControl.Get()) {
    case ResizeValue::None:
        configuredCamera.resizeOptions.enable = false;
        configuredCamera.resizeOptions.width = 0;
        configuredCamera.resizeOptions.height = 0;
        break;
    case ResizeValue::FullHd:
        configuredCamera.resizeOptions.enable = true;
        configuredCamera.resizeOptions.width = FullHdFrameSize().GetWidth();
        configuredCamera.resizeOptions.height = FullHdFrameSize().GetHeight();
        break;
    case ResizeValue::Hd:
        configuredCamera.resizeOptions.enable = true;
        configuredCamera.resizeOptions.width = HdFrameSize().GetWidth();
        configuredCamera.resizeOptions.height = HdFrameSize().GetHeight();
        break;
    case ResizeValue::Vga:
        configuredCamera.resizeOptions.enable = true;
        configuredCamera.resizeOptions.width = VgaFrameSize().GetWidth();
        configuredCamera.resizeOptions.height = VgaFrameSize().GetHeight();
        break;
    }
}

void CameraConfigurator::ConfigureControl(IControl *control, std::string camera)
{
    if (control->IsMenu()) {
        ConfigureMenu(control, camera);
    } else {
        ConfigureInteger(control, camera);
    }
}

void CameraConfigurator::ConfigureMenu(IControl *control, std::string camera)
{
    auto prompt = "Select value of " + std::string(control->GetName()) + " on " + camera;

    auto controlMenu = control->GetMenuEntries();
    std::vector<DisplayMenuEntry<int32_t>> menu;
    std::transform(controlMenu.begin(), controlMenu.end(), std::back_inserter(menu),
        [](MenuEntry &controlEntry) {
            return DisplayMenuEntry<int32_t>(controlEntry.name, controlEntry.index);
        }
    );

    int32_t selected;
    if (GetMenuEntry(prompt, menu, selected)) {
        !control->Set(selected);
    }
}

void CameraConfigurator::ConfigureInteger(IControl *control, std::string camera)
{
    auto prompt = "Enter value of " + std::string(control->GetName()) + " on " + camera;
    int32_t selected;
    if (GetInteger(prompt, control->GetMinValue(), control->GetMaxValue(), control->GetStepValue(), control->GetDefaultValue(), selected)) {
        control->Set(selected);
    }
}

bool CameraConfigurator::GetConfigurationMenuEntry(int32_t &index)
{
    std::vector<DisplayMenuEntry<int32_t>> menu;
    for (uint32_t i = 0; i < cameras.size(); ++i) {
        auto description = std::string(cameras[i]->GetName()) + " (" + cameras[i]->GetDriverName() + ")";
        menu.emplace_back(DisplayMenuEntry<int32_t>(description, i));
    }

    return GetMenuEntry("Configuration menu", menu, index);
}

bool CameraConfigurator::GetControlEntry(ICamera *camera, IControl *&control)
{
    IControlList controls = camera->GetControlList();

    controls.erase(std::remove_if(controls.begin(), controls.end(), [](IControl *control){ return std::string(control->GetName()) == "Frame Interval"; }));

    controls.push_back(&debayeringControl);
    controls.push_back(&resizeControl);

    std::vector<DisplayMenuEntry<IControl*>> menu;
    std::transform(controls.begin(), controls.end(), std::back_inserter(menu),
        [this](IControl* control) {
            auto description = std::string(control->GetName()) + ": " + GetCurrentControlValue(control);
            return DisplayMenuEntry<IControl*>(description, control);
        }
    );

    return GetMenuEntry(std::string("Control menu for ") + camera->GetName(), menu, control);
}

std::string CameraConfigurator::GetCurrentControlValue(IControl *control)
{
    if (control->IsMenu()) {
        return (control->GetMenuEntries()[control->Get()].name);
    } else {
        return std::to_string(control->Get());
    }
}

}