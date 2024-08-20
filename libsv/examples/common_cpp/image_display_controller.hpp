#pragma once

#include "resize_options.hpp"

namespace common
{
    class ImageDisplayController
    {
        public:
            virtual void SetDebayer(bool enable) = 0;
            virtual void SetResizeOptions(ResizeOptions resizeOptions) = 0;
            virtual void ToggleCrosshair() = 0;
            virtual void ToggleShowFps() = 0;
            virtual void SetFps(uint32_t acquisitionFps, uint32_t displayFps) = 0;
    };
}