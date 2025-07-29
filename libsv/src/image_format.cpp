#include "image_format.hpp"
#include "v4l2_formats.hpp"
#include "buffer_info.hpp"
#include "control_menu.hpp"
#include "easylogging++/easylogging++.h"
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <sys/ioctl.h>
#include <cstring>

ImageFormat::ImageFormat(int fd) : fd(fd) 
{
    bufferInfo = std::unique_ptr<BufferInfo>(new BufferInfo());
}

ControlMenu ImageFormat::EnumerateImageFormat()
{
    ControlMenu menu;

    uint32_t index = 0;
    uint32_t format;
    while(EnumerateImageFormat(index, &format)) {
        ControlMenuEntry entry = {};
        entry.name = v4l2::format::GetPixelFormatDescription(format);
        entry.index = index;
        menu.push_back(entry);
        ++index;
    }

    if (menu.empty())
        LOG(ERROR) << "failed to find available image formats";

    return menu;
}

bool ImageFormat::EnumerateImageFormat(uint32_t index, uint32_t *format)
{
    v4l2_fmtdesc formatDescription = {};

    formatDescription.index = index;
    formatDescription.type = bufferInfo->GetType();

    if (::ioctl(fd, VIDIOC_ENUM_FMT, &formatDescription)) {
        return false;
    }

    if (format != nullptr) {
        *format = formatDescription.pixelformat;
    }

    return true;
}

uint32_t ImageFormat::GetImageFormatIndex(uint32_t format)
{
    uint32_t index = 0;
    uint32_t formatIterator;
    
    while(EnumerateImageFormat(index, &formatIterator)) {
        if (format == formatIterator) {
            return index;
        }
        ++index;
    }

    LOG(ERROR) << "failed to find image format index, using default";
    return 0;
}

uint32_t ImageFormat::GetImageFormat(uint32_t index)
{
    uint32_t format;

    if (EnumerateImageFormat(index, &format)) {
        return format;
    }

    LOG(ERROR) << "failed to find image format by index, using default";
    return 0;
}

MplaneSubdevImageFormat::MplaneSubdevImageFormat(int fd) : ImageFormat(fd)
{
    bufferInfo = std::unique_ptr<BufferInfo>(new MplaneBufferInfo());
}

bool MplaneSubdevImageFormat::EnumerateImageFormat(uint32_t index, uint32_t *format)
{
    v4l2_subdev_mbus_code_enum formatDescription = {};

    formatDescription.index = index;
    formatDescription.which = V4L2_SUBDEV_FORMAT_ACTIVE;

    if (::ioctl(fd, VIDIOC_SUBDEV_ENUM_MBUS_CODE, &formatDescription)) {
        return false;
    }

    if (format != nullptr) {
        *format = v4l2::format::TranslateToDev(formatDescription.code);
    }

    return true;
}