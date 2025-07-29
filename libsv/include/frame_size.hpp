#pragma once

#include "control_menu.hpp"

class FrameSize
{
    public:
        FrameSize(int fd);
        ControlMenu EnumerateFrameSize(uint32_t pixelFormat);
        uint32_t GetFrameSizeIndex(uint32_t pixelFormat, uint32_t width, uint32_t height);
        uint32_t GetWidth(uint32_t pixelFormat, uint32_t index);
        uint32_t GetHeight(uint32_t pixelFormat, uint32_t index);

    private:
        virtual bool EnumerateFrameSize(uint32_t pixelFormat, uint32_t index, uint32_t *width, uint32_t *height);

    protected:
        int fd;
};

class MplaneSubdevFrameSize : public FrameSize
{
    public:
        MplaneSubdevFrameSize(int fd);

    private:
        bool EnumerateFrameSize(uint32_t pixelFormat, uint32_t index, uint32_t *width, uint32_t *height) override;
};