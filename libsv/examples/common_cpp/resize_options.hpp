#pragma once

#include <stdint.h>

namespace common
{
    struct ResizeOptions {
        bool enable;
        uint32_t width;
        uint32_t height;
    };
}