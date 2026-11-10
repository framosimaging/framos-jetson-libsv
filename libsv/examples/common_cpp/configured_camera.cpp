#include "configured_camera.hpp"
#include <cstring>

namespace common
{

IControl * getCameraControlByName(ICamera * camera, const char * name)
{
    for (auto &control : camera->GetControlList())
    {
        if (strcmp(control->GetName(), name) == 0)
        { 
            return control;
        }
    }
    return nullptr;
}

int64_t getCameraControlValueByName(ICamera * camera, const char * name)
{
    IControl * control = getCameraControlByName(camera, name);

    return control->Get();
}

}
