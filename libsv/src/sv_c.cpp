#include "sv/sv.h"
#include "globals.hpp"
#include <cstdlib>

// C

const char* sv_camera_GetName(ICamera* camera)
{
    return camera->GetName();
}

const char* sv_camera_GetDriverName(ICamera* camera)
{
    return camera->GetDriverName();
}

int sv_camera_StartStream(ICamera* camera)
{
    return camera->StartStream();
}

int sv_camera_StopStream(ICamera* camera)
{
    return camera->StopStream();
}

IImage sv_camera_GetImage(ICamera* camera)
{
    return camera->GetImage();
}

int sv_camera_ReturnImage(ICamera* camera, IImage image)
{
    return camera->ReturnImage(image);
}

IImageInfo sv_camera_GetImageInfo(ICamera* camera)
{
    return camera->GetImageInfo();
}

CIControlList sv_camera_GetControlList(ICamera* camera, int* size)
{
    auto controlList = camera->GetControlList();

    if (controlList.size() == 0) {
        *size = 0;
        return nullptr;
    }

    auto controls = (IControl**)malloc(controlList.size() * sizeof(IControl*));

    for (unsigned int i = 0; i < controlList.size(); ++i) {
        controls[i] = controlList[i];
    }

    *size = controlList.size();

    return controls;
}

uint32_t sv_control_GetID(IControl* control)
{
    return control->GetID();
}

const char* sv_control_GetName(IControl* control)
{
    return control->GetName();
}

int32_t sv_control_Get(IControl* control)
{
    return control->Get();
}

int sv_control_Set(IControl* control, int32_t val)
{
    return control->Set(val);
}

int32_t sv_control_GetMinValue(IControl* control)
{
    return control->GetMinValue();
}

int32_t sv_control_GetMaxValue(IControl* control)
{
    return control->GetMaxValue();
}

int32_t sv_control_GetStepValue(IControl* control)
{
    return control->GetStepValue();
}

int32_t sv_control_GetDefaultValue(IControl* control)
{
    return control->GetDefaultValue();
}

CMenuEntryList sv_control_GetMenuEntries(IControl* control, int* size)
{
    auto menuList = control->GetMenuEntries();
    if (menuList.size() == 0) {
        *size = 0;
        return nullptr;
    }

    auto menus = (MenuEntry*)malloc(menuList.size() * sizeof(MenuEntry));
    for (unsigned int i = 0; i < menuList.size(); ++i) {
        menus[i] = menuList[i];
    }

    *size = menuList.size();
    return menus;
}

int sv_control_IsMenu(IControl* control)
{
    return control->IsMenu();
}

CICameraList sv_GetAllCameras(int* size)
{
    auto icameraList = sv::GetAllCameras();
    if (icameraList.size() == 0) {
        *size = 0;
        return nullptr;
    }

    auto cameraList = (ICamera**)malloc(icameraList.size() * sizeof(ICamera*));

    for (unsigned int i = 0; i < icameraList.size(); ++i) {
        cameraList[i] = icameraList[i];
    }

    *size = icameraList.size();

    return cameraList;
}

ICamera* sv_GetCamera(const char* node)
{
    return sv::GetCamera(node);
}

IControl* sv_camera_GetControl(ICamera* camera, int id)
{
    return camera->GetControl(id);
}

IProcessedImage sv_AllocateProcessedImage(IImageInfo imageInfo)
{
    return sv::AllocateProcessedImage(imageInfo);
}

bool sv_ProcessImage(const IImage *input, IProcessedImage *output, SV_PLATFORM_PROCESSING algorithm)
{
    return sv::ProcessImage(*input, *output, algorithm);
}

void sv_DeallocateProcessedImage(IProcessedImage *image)
{
    return sv::DeallocateProcessedImage(*image);
}

SV_PLATFORM sv_GetPlatform()
{
    return sv::GetPlatform();
}