#include "int64_control.hpp"
#include "platform.hpp"
#include "easylogging++/easylogging++.h"
#include <sys/ioctl.h>

Int64Control::Int64Control(int fd, v4l2_queryctrl control) : V4l2Control(fd, control.id, std::string((char*)control.name))
{
    QueryControl();

    switch (v4l2::platform) {
    case SV_PLATFORM_JETSON_TX2:
    case SV_PLATFORM_JETSON_XAVIER:
    case SV_PLATFORM_JETSON_NANO:
    case SV_PLATFORM_JETSON_XAVIER_NX:
    case SV_PLATFORM_JETSON_TX2NX:
    case SV_PLATFORM_JETSON_AGX_ORIN:
    case SV_PLATFORM_JETSON_ORIN_NANO:
    case SV_PLATFORM_JETSON_ORIN_NX:
        Set(defaultValue);
        break;
    default:
        break;
    }
}

int64_t Int64Control::Get()
{
    v4l2_ext_control control = {};
    control.id = id;
    control.size = 0;

    v4l2_ext_controls controls = {};
    controls.count = 1;
    controls.controls = &control;

    if (::ioctl(fd, VIDIOC_G_EXT_CTRLS, &controls) != 0) {
        LOG(ERROR) << "VIDIOC_G_EXT_CTRLS failed";
        return 0;
    }

    return controls.controls[0].value64;
}

bool Int64Control::SetValue(int64_t val)
{
    v4l2_ext_control control = {};
    control.id = id;
    control.size = 0;
    control.value64 = val;

    v4l2_ext_controls controls = {};
    controls.count = 1;
    controls.controls = &control;

    return ::ioctl(fd, VIDIOC_S_EXT_CTRLS, &controls) == 0;
}

void Int64Control::Update()
{
    QueryControl();
}

void Int64Control::QueryControl()
{
    v4l2_query_ext_ctrl extControl = {};
    extControl.id = id;

    if (::ioctl(fd, VIDIOC_QUERY_EXT_CTRL, &extControl) == 0) {
        minValue = extControl.minimum;
        maxValue = extControl.maximum;
        stepValue = extControl.step;
        defaultValue = extControl.default_value;
    } else {
        LOG(ERROR) << "VIDIOC_QUERY_EXT_CTRL failed";
    }
}
