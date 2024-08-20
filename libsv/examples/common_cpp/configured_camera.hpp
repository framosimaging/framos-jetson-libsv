#pragma once

#include "sv/sv.h"
#include "resize_options.hpp"

namespace common
{

struct ConfiguredCamera
{
    ICamera *camera;
    bool debayering;
    ResizeOptions resizeOptions;
};

using ConfiguredCameras = std::vector<ConfiguredCamera>;


/* helper functions for accessing camera controls */

IControl * getCameraControlByName(ICamera * camera, const char * name);
int64_t getCameraControlValueByName(ICamera * camera, const char * name);

}