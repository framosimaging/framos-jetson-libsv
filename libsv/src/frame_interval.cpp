#include "frame_interval.hpp"
#include "control_menu.hpp"
#include "easylogging++/easylogging++.h"
#include <cstring>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <sys/ioctl.h>

FrameInterval::FrameInterval(int fd) : fd(fd) {}

ControlMenu FrameInterval::EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height)
{
    ControlMenu menu;

    uint32_t index = 0;
    uint32_t fps;
    while(EnumerateFrameInterval(pixelFormat, width, height, index, &fps)) {
        ControlMenuEntry entry;
        entry.index = index;
        entry.name = std::to_string(fps);
        menu.push_back(entry);
        ++index;
    }

    return menu;
}

bool FrameInterval::EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height, uint32_t index, uint32_t *fps)
{
    v4l2_frmivalenum frameInterval = {};

    frameInterval.index = index;
    frameInterval.pixel_format = pixelFormat;
    frameInterval.width = width;
    frameInterval.height = height;

    if (::ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInterval)) {
        return false;
    }

    *fps = frameInterval.discrete.denominator;

    return true;
}

SubdevFrameInterval::SubdevFrameInterval(int fd) : FrameInterval(fd) {}

bool SubdevFrameInterval::EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height, uint32_t index, uint32_t *fps)
{
    v4l2_subdev_frame_interval_enum frameInterval = {};

    frameInterval.index = index;
    frameInterval.code = pixelFormat;
    frameInterval.width = width;
    frameInterval.height = height;
    frameInterval.which = V4L2_SUBDEV_FORMAT_ACTIVE;

    if (::ioctl(fd, VIDIOC_SUBDEV_ENUM_FRAME_INTERVAL, &frameInterval)) {
        return false;
    }

    *fps = frameInterval.interval.denominator;

    return true;
}