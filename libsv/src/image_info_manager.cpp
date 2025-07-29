#include "image_info_manager.hpp"
#include "v4l2_formats.hpp"
#include "platform.hpp"
#include "linux.hpp"

#include "easylogging++/easylogging++.h"

#include <sys/ioctl.h>
#include <linux/v4l2-subdev.h>

ImageInfoManager::ImageInfoManager(int fd) : fd(fd) 
{
    UpdateFromDevice();
}

void ImageInfoManager::UpdateFromDevice()
{
    v4l2_format format = {};

    format.type = GetType();

    if (::ioctl(fd, VIDIOC_G_FMT, &format)) {
        LOG(ERROR) << "VIDIOC_G_FMT failed";
        return;
    }

    UpdateFormat(format);
}

bool ImageInfoManager::SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight)
{
    v4l2_format formatDescriptor = {};

    formatDescriptor.type = GetType();

    formatDescriptor.fmt.pix.pixelformat = newFormat;
    formatDescriptor.fmt.pix.width = newWidth;
    formatDescriptor.fmt.pix.height = newHeight;

    return ::ioctl(fd, VIDIOC_S_FMT, &formatDescriptor) == 0;
}

bool ImageInfoManager::SetPixelFormat(uint32_t newFormat)
{
    return SetFormat(newFormat, imageInfo.width, imageInfo.height);
}

bool ImageInfoManager::SetFrameSize(uint32_t newWidth, uint32_t newHeight)
{
    return SetFormat(imageInfo.pixelFormat, newWidth, newHeight);
}

IImageInfo ImageInfoManager::GetImageInfo()
{
    return imageInfo;
}

void ImageInfoManager::UpdateFormat(v4l2_format &format)
{
    imageInfo.height = format.fmt.pix.height;
    imageInfo.width = format.fmt.pix.width;
    imageInfo.pixelFormat = format.fmt.pix.pixelformat;
    imageInfo.stride = format.fmt.pix.bytesperline;
    imageInfo.length = imageInfo.stride * imageInfo.height;
}

bool ImageInfoManager::SetFormat(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight)
{
    if (SetFormatToDevice(newFormat, newWidth, newHeight)) {
        
        UpdateFromDevice();
        return true;
    }

    LOG(ERROR) << "failed to set format to device";

    return false;
}

v4l2_buf_type ImageInfoManager::GetType()
{
    return V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

MplaneImageInfoManager::MplaneImageInfoManager(int fd) : ImageInfoManager(fd) 
{
    UpdateFromDevice();
}

void MplaneImageInfoManager::UpdateFormat(v4l2_format &format)
{
    imageInfo.height = format.fmt.pix_mp.height;
    imageInfo.width = format.fmt.pix_mp.width;
    imageInfo.pixelFormat = format.fmt.pix_mp.pixelformat;
    imageInfo.stride = format.fmt.pix_mp.plane_fmt[0].bytesperline;
    imageInfo.length = imageInfo.stride * imageInfo.height;
}

bool MplaneImageInfoManager::SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight)
{
    v4l2_format formatDescriptor = {};

    formatDescriptor.type = GetType();

    formatDescriptor.fmt.pix_mp.pixelformat = newFormat;
    formatDescriptor.fmt.pix_mp.width = newWidth;
    formatDescriptor.fmt.pix_mp.height = newHeight;

    return ::ioctl(fd, VIDIOC_S_FMT, &formatDescriptor) == 0;
}

v4l2_buf_type MplaneImageInfoManager::GetType()
{
    return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
}

MplaneSubdevImageInfoManager::MplaneSubdevImageInfoManager(int fd, int subdevFd, std::vector<int> pipeline) 
: MplaneImageInfoManager(fd), subdevFd(subdevFd), pipeline(pipeline) {}

bool MplaneSubdevImageInfoManager::SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight)
{
    v4l2_subdev_format formatDescriptor = {};

    formatDescriptor.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    formatDescriptor.format.code = v4l2::format::TranslateToSubdev(newFormat);
    formatDescriptor.format.width = newWidth;
    formatDescriptor.format.height = newHeight;

    if (::ioctl(subdevFd, VIDIOC_SUBDEV_S_FMT, &formatDescriptor)) {
        LOG(ERROR) << "VIDIOC_SUBDEV_S_FMT failed";
        return false;
    }

    for (auto node : pipeline) {
        if (::ioctl(node, VIDIOC_SUBDEV_S_FMT, &formatDescriptor)) {
            LOG(ERROR) << "VIDIOC_SUBDEV_S_FMT on pipeline elements failed";
            return false;
        }
    }

    /**
     * On DragonBoard 410c the formats in image pipeline are sometimes not properly updated.
     * Reopening the stream file descriptor is a workaround for this issue.
     */
    if (v4l2::platform == SV_PLATFORM_DRAGONBOARD_410C) 
        fd = linux::ReopenFileDescriptor(fd);

    return MplaneImageInfoManager::SetFormatToDevice(
        v4l2::format::TranslateToDev(formatDescriptor.format.code), 
        formatDescriptor.format.width, 
        formatDescriptor.format.height
    );
}
