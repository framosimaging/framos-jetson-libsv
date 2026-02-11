#include "int32_control.hpp"
#include "platform.hpp"
#include "easylogging++/easylogging++.h"
#include <sys/ioctl.h>

Int32Control::Int32Control(int fd, v4l2_queryctrl control) : V4l2Control(fd, control)
{
    if (IsArgusControl(name)) {
        Set(defaultValue);
    }
}

int64_t Int32Control::Get()
{
    v4l2_control control = {};
    control.id = id;
    control.value = 0;
    if(::ioctl(fd, VIDIOC_G_CTRL, &control) != 0) {
        LOG(ERROR) << "VIDIOC_G_CTRL failed";
        return 0;
    }
    return control.value;
}
    
bool Int32Control::SetValue(int64_t val)
{
    v4l2_control control = {};
    
    control.id = id;
    control.value = val;
    return ::ioctl(fd, VIDIOC_S_CTRL, &control) == 0;
}

bool Int32Control::IsArgusControl(std::string name)
{
    switch (v4l2::platform) {
        case SV_PLATFORM_JETSON_TX2:
        case SV_PLATFORM_JETSON_XAVIER:
        case SV_PLATFORM_JETSON_NANO:
        case SV_PLATFORM_JETSON_XAVIER_NX:
        case SV_PLATFORM_JETSON_TX2NX:
        case SV_PLATFORM_JETSON_AGX_ORIN:
        case SV_PLATFORM_JETSON_AGX_THOR:
        case SV_PLATFORM_JETSON_ORIN_NANO:
        case SV_PLATFORM_JETSON_ORIN_NX:
            return name == "Bypass Mode" || name == "Override Enable";
        default:
            return false;
    }
}
