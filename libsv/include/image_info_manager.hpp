#pragma once

#include "sv/sv.h"
#include <cinttypes>
#include <linux/videodev2.h>

class FormatInfo;

class ImageInfoManager
{
    public:
        explicit ImageInfoManager(int fd);
        void UpdateFromDevice();
        bool SetPixelFormat(uint32_t newFormat);
        bool SetFrameSize(uint32_t newWidth, uint32_t newHeight);
        IImageInfo GetImageInfo();

    private:
        bool SetFormat(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight);
        virtual void UpdateFormat(v4l2_format &format);
        virtual v4l2_buf_type GetType();

    protected:
        int fd;
        IImageInfo imageInfo = {};
        virtual bool SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight);
};

class MplaneImageInfoManager : public ImageInfoManager
{
    public:
        explicit MplaneImageInfoManager(int fd);
        void UpdateFormat(v4l2_format &format) override;

    protected:
        virtual bool SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight) override;

    private:
        v4l2_buf_type GetType() override;
};

class MplaneSubdevImageInfoManager : public MplaneImageInfoManager
{
    public:
        MplaneSubdevImageInfoManager(int fd, int subdevFd, std::vector<int> pipeline);
        
    private:
        int subdevFd;
        std::vector<int> pipeline;

    protected:
        bool SetFormatToDevice(uint32_t newFormat, uint32_t newWidth, uint32_t newHeight) override;
};