#pragma once

#include "control_menu.hpp"
#include <cinttypes>
#include <memory>

class BufferInfo;

class ImageFormat
{
    public:
        explicit ImageFormat(int fd);
        ControlMenu EnumerateImageFormat();
        uint32_t GetImageFormatIndex(uint32_t format);
        uint32_t GetImageFormat(uint32_t index);
    
    private:
        virtual bool EnumerateImageFormat(uint32_t index, uint32_t *format = nullptr);

    protected:
        int fd;
        std::unique_ptr<BufferInfo> bufferInfo;
};

class MplaneSubdevImageFormat : public ImageFormat
{
    public:
        explicit MplaneSubdevImageFormat(int fd);
    
    private:
        bool EnumerateImageFormat(uint32_t index, uint32_t *format = nullptr) override;
};