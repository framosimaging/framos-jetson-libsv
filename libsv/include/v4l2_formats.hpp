#pragma once

#include <cinttypes>
#include <string>

namespace v4l2
{
    namespace format
    {
        uint32_t GetBpp(uint32_t format);
        uint32_t TranslateToDev(uint32_t subdevFormat);
        uint32_t TranslateToSubdev(uint32_t subdevFormat);
        std::string GetPixelFormatDescription(uint32_t format);
        std::string DecodeFourCc(uint32_t format);
    }
}