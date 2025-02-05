#pragma once

#include <inttypes.h>

namespace common
{
    class FrameSize
    {
        public:
            explicit FrameSize(uint32_t width, uint32_t height) 
            {
                this->width = width;
                this->height = height;
            }

            virtual ~FrameSize()
            {
                
            }

            uint32_t GetWidth() 
            {
                return width;
            }

            uint32_t GetHeight()
            {
                return height;
            }

            bool operator > (const FrameSize &other) const 
            {
                if (width > other.width && height > other.height)
                    return true;
                
                if (width > other.width && height == other.height)
                    return true;
                
                if (width == other.width && height > other.height)
                    return true;
                
                return false;
            }

            bool operator == (const FrameSize &other) const
            {
                if (width == other.width && height == other.height) {
                    return true;
                }

                return false;
            }

            bool operator != (const FrameSize &other) const
            {
                return !(*this == other);
            }

            bool operator >= (const FrameSize &other) const
            {
                if (*this == other || *this > other) {
                    return true;
                }

                return false;
            }
        
        private:
            uint32_t width;
            uint32_t height;
    };

    class FullHdFrameSize : public FrameSize
    {
        public:
            explicit FullHdFrameSize() : FrameSize(1920, 1080)
            {

            }
    };

    class HdFrameSize : public FrameSize
    {
        public:
            explicit HdFrameSize() : FrameSize(1280, 720)
            {

            }
    };

    class VgaFrameSize : public FrameSize
    {
        public:
            explicit VgaFrameSize(): FrameSize(640, 480)
            {

            }
    };

    class InvalidFrameSize : public FrameSize
    {
        public:
            explicit InvalidFrameSize(): FrameSize(0, 0)
            {

            }
    };
}