#include "frame_size.hpp"
#include "v4l2_formats.hpp"
#include "control_menu.hpp"
#include "easylogging++/easylogging++.h"
#include <cstring>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <sys/ioctl.h>

FrameSize::FrameSize(int fd) : fd(fd) {}

ControlMenu FrameSize::EnumerateFrameSize(uint32_t pixelFormat)
{
    ControlMenu menu;

    uint32_t index = 0;                
    uint32_t width;
    uint32_t height;
    while (EnumerateFrameSize(pixelFormat, index, &width, &height)) {
        ControlMenuEntry entry = {};
        entry.index = index;
        entry.name = (std::to_string(width) + " x "+ std::to_string(height)).c_str();
        menu.push_back(entry);
        ++index;
    }

    return menu;
}

bool FrameSize::EnumerateFrameSize(uint32_t pixelFormat, uint32_t index, uint32_t *width, uint32_t *height)
{
    v4l2_frmsizeenum frameSize = {};
            
    frameSize.index = index;
    frameSize.pixel_format = pixelFormat; 

    if (::ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) {
        return false;
    }            

    *width = frameSize.discrete.width;
    *height = frameSize.discrete.height;

    return true;
}

uint32_t FrameSize::GetFrameSizeIndex(uint32_t pixelFormat, uint32_t width, uint32_t height)
{
    uint32_t index = 0;
    uint32_t widthIterator;
    uint32_t heightIterator;
    while (EnumerateFrameSize(pixelFormat, index, &widthIterator, &heightIterator)) {
        if (widthIterator == width && heightIterator == height) {
            return index;
        }
        ++index;
    }

    LOG(WARNING) << "frame size not found, using default";
    return 0;
}

uint32_t FrameSize::GetWidth(uint32_t pixelFormat, uint32_t index)
{
    uint32_t width;
    uint32_t height;

    if (EnumerateFrameSize(pixelFormat, index, &width, &height)) {
        return width;
    }

    LOG(WARNING) << "frame width not found, using default";

    return 0;
}

uint32_t FrameSize::GetHeight(uint32_t pixelFormat, uint32_t index)
{
    uint32_t width;
    uint32_t height;

    if (EnumerateFrameSize(pixelFormat, index, &width, &height)) {
        return height;
    }

    LOG(WARNING) << "frame height not found, using default";

    return 0;
}

MplaneSubdevFrameSize::MplaneSubdevFrameSize(int fd) : FrameSize(fd) {}

bool MplaneSubdevFrameSize::EnumerateFrameSize(uint32_t pixelFormat, uint32_t index, uint32_t *width, uint32_t *height)
{
    v4l2_subdev_frame_size_enum frameSize = {};

    frameSize.index = index;
    frameSize.pad = 0;
    frameSize.code = v4l2::format::TranslateToSubdev(pixelFormat);

    if (::ioctl(fd, VIDIOC_SUBDEV_ENUM_FRAME_SIZE, &frameSize)) {
        return false;
    }

    *width = frameSize.max_width;
    *height = frameSize.max_height;

    return true;
}