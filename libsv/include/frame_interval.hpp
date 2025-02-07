#pragma once

#include "control_menu.hpp"

class FrameInterval
{
    public:
        FrameInterval(int fd);
        ControlMenu EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height);

    private:
        virtual bool EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height, uint32_t index, uint32_t *fps);

    protected:
        int fd;
};

class SubdevFrameInterval : public FrameInterval
{
    public:
        SubdevFrameInterval(int fd);

    private:
        bool EnumerateFrameInterval(uint32_t pixelFormat, uint32_t width, uint32_t height, uint32_t index, uint32_t *fps) override;
};