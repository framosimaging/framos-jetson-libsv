#include "v4l2_formats.hpp"

#include "easylogging++/easylogging++.h"

#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <sstream>

#ifndef V4L2_PIX_FMT_SBGGR12P
#define V4L2_PIX_FMT_SBGGR12P v4l2_fourcc('p', 'B', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SGBRG12P
#define V4L2_PIX_FMT_SGBRG12P v4l2_fourcc('p', 'G', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SGRBG12P
#define V4L2_PIX_FMT_SGRBG12P v4l2_fourcc('p', 'g', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SRGGB12P
#define V4L2_PIX_FMT_SRGGB12P v4l2_fourcc('p', 'R', 'C', 'C')
#endif

namespace v4l2 
{
    namespace format
    {
        uint32_t GetBpp(uint32_t pixelformat)
        {
            switch(TranslateToDev(pixelformat)) {
            case V4L2_PIX_FMT_SBGGR12P:
            case V4L2_PIX_FMT_SGBRG12P:
            case V4L2_PIX_FMT_SGRBG12P:
            case V4L2_PIX_FMT_SRGGB12P:
            case V4L2_PIX_FMT_SBGGR12:
            case V4L2_PIX_FMT_SGBRG12:
            case V4L2_PIX_FMT_SGRBG12:
            case V4L2_PIX_FMT_SRGGB12:
                return 12;
            case V4L2_PIX_FMT_SBGGR10P:
            case V4L2_PIX_FMT_SGBRG10P:
            case V4L2_PIX_FMT_SGRBG10P:
            case V4L2_PIX_FMT_SRGGB10P:
            case V4L2_PIX_FMT_SBGGR10:
            case V4L2_PIX_FMT_SGBRG10:
            case V4L2_PIX_FMT_SGRBG10:
            case V4L2_PIX_FMT_SRGGB10:
                return 10;
            case V4L2_PIX_FMT_SBGGR8:
            case V4L2_PIX_FMT_SGBRG8:
            case V4L2_PIX_FMT_SGRBG8:
            case V4L2_PIX_FMT_SRGGB8:
                return 8;
            }

            LOG(ERROR) << "unknown pixel format, assuming 8bit";
            return 8;
        }

        uint32_t TranslateToDev(uint32_t subdevFormat)
        {
            switch(subdevFormat) {
            case MEDIA_BUS_FMT_SBGGR12_1X12:
                return V4L2_PIX_FMT_SBGGR12P;
            case MEDIA_BUS_FMT_SGBRG12_1X12:
                return V4L2_PIX_FMT_SGBRG12P;
            case MEDIA_BUS_FMT_SGRBG12_1X12:
                return V4L2_PIX_FMT_SGRBG12P;
            case MEDIA_BUS_FMT_SRGGB12_1X12:
                return V4L2_PIX_FMT_SRGGB12P;
            case MEDIA_BUS_FMT_SBGGR10_1X10:
                return V4L2_PIX_FMT_SBGGR10P;
            case MEDIA_BUS_FMT_SGBRG10_1X10:
                return V4L2_PIX_FMT_SGBRG10P;
            case MEDIA_BUS_FMT_SGRBG10_1X10:
                return V4L2_PIX_FMT_SGRBG10P;
            case MEDIA_BUS_FMT_SRGGB10_1X10:
                return V4L2_PIX_FMT_SRGGB10P;
            case MEDIA_BUS_FMT_SBGGR8_1X8:
                return V4L2_PIX_FMT_SBGGR8;
            case MEDIA_BUS_FMT_SGBRG8_1X8:
                return V4L2_PIX_FMT_SGBRG8;
            case MEDIA_BUS_FMT_SGRBG8_1X8:
                return V4L2_PIX_FMT_SGRBG8;
            case MEDIA_BUS_FMT_SRGGB8_1X8:
                return V4L2_PIX_FMT_SRGGB8;
            default:
                return subdevFormat;
            }
        }

        uint32_t TranslateToSubdev(uint32_t devFormat)
        {
            switch(devFormat) {
            case V4L2_PIX_FMT_SBGGR12P:
                return MEDIA_BUS_FMT_SBGGR12_1X12;
            case V4L2_PIX_FMT_SGBRG12P:
                return MEDIA_BUS_FMT_SGBRG12_1X12;
            case V4L2_PIX_FMT_SGRBG12P:
                return MEDIA_BUS_FMT_SGRBG12_1X12;
            case V4L2_PIX_FMT_SRGGB12P:
                return MEDIA_BUS_FMT_SRGGB12_1X12;
            case V4L2_PIX_FMT_SBGGR10P:
                return MEDIA_BUS_FMT_SBGGR10_1X10;
            case V4L2_PIX_FMT_SGBRG10P:
                return MEDIA_BUS_FMT_SGBRG10_1X10;
            case V4L2_PIX_FMT_SGRBG10P:
                return MEDIA_BUS_FMT_SGRBG10_1X10;
            case V4L2_PIX_FMT_SRGGB10P:
                return MEDIA_BUS_FMT_SRGGB10_1X10;
            case V4L2_PIX_FMT_SBGGR8:
                return MEDIA_BUS_FMT_SBGGR8_1X8;
            case V4L2_PIX_FMT_SGBRG8:
                return MEDIA_BUS_FMT_SGBRG8_1X8;
            case V4L2_PIX_FMT_SGRBG8:
                return MEDIA_BUS_FMT_SGRBG8_1X8;
            case V4L2_PIX_FMT_SRGGB8:
                return MEDIA_BUS_FMT_SRGGB8_1X8;
            default:
                return devFormat;
            }
        }

        std::string GetPixelFormatDescription(uint32_t format)
        {
            std::stringstream description; 

            description << GetBpp(format) << "bit";

            description << " (" << DecodeFourCc(TranslateToDev(format)) << ")";

            return description.str();
        }

        std::string DecodeFourCc(uint32_t format)
        {
            std::string result(4, ' ');

            result[0] = format & 0xFF;
            result[1] = (format >> 8) & 0xFF;
            result[2] = (format >> 16) & 0xFF;
            result[3] = (format >> 24) & 0xFF;

            return result;
        }     
    }   
}