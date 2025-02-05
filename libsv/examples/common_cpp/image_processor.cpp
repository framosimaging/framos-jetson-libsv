#include "image_processor.hpp"
#include "image_util.hpp"
#include <linux/videodev2.h>

namespace common 
{

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

ImageProcessor::ImageProcessor(uint32_t pixelFormat) 
: debayer(false), resizeOptions({}), showCrosshair(false), showFps(true), acquisitionFps(0), displayFps(0), pixelFormat(pixelFormat)
{

}

ImageProcessor::~ImageProcessor()
{
    
}

void ImageProcessor::SetDebayer(bool enable)
{
    debayer = enable;
}

void ImageProcessor::SetResizeOptions(ResizeOptions resizeOptions)
{
    this->resizeOptions = resizeOptions;
}

void ImageProcessor::ToggleCrosshair()
{
    showCrosshair = !showCrosshair;
}

void ImageProcessor::ToggleShowFps()
{
    showFps = !showFps;
}

void ImageProcessor::SetFps(uint32_t acquisitionFps, uint32_t displayFps)
{   
    this->acquisitionFps = acquisitionFps;
    this->displayFps = displayFps;
}

void ImageProcessor::SetPixelFormat(uint32_t pixelFormat)
{
    pixelFormat = pixelFormat;
}

void ImageProcessor::ProcessImage(const IProcessedImage &input, cv::UMat &output)
{
    AllocateMat(input, output);

    if (debayer) {       

        /**
         * OpenCV function imshow implicitly converts the image bit depth down to 8 bit before displaying the image.
         * This results in data loss that is not visible to the human eye.
         * Converting the image bit depth manually before performing image processing can lead to better performance.
         * Another advantage of this approach is that it facilitates pipelining by separating the tasks of converting
         * the image bit depth from displaying the image.
         *
         * In this case, the image bit depth is manually converted down to 8bit before further processing. This increases 
         * performance because processing is faster to perform on a smaller 8 bit image.
         */
        ConvertTo8Bit(output);
        DebayerImage(output, pixelFormat);
    }

    if (resizeOptions.enable)
        ResizeImage(output, resizeOptions);

    if (showCrosshair)
        DrawCrosshair(output);

    if (showFps)
        DrawFps(output, acquisitionFps, displayFps);
}

void ImageProcessor::AllocateMat(const IProcessedImage &image, cv::UMat &output)
{
    int8_t bpp = GetBpp(image.pixelFormat);

    int type = bpp == 8 ? CV_8U : CV_16U;
    
    cv::Mat(image.height, image.width, type, image.data).copyTo(output);
}

uint8_t ImageProcessor::GetBpp(uint32_t pixelFormat)
{
    switch (pixelFormat) {
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
        return 8;
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR10P:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGBRG10P:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SGRBG10P:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SRGGB10P:
        return 10;
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR12P:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGBRG12P:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SGRBG12P:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SRGGB12P:
        return 12;
    }

    throw std::invalid_argument("Could not detect bit depth based on pixel format " + std::to_string(pixelFormat));
}

void ImageProcessor::DebayerImage(cv::UMat &mat, uint32_t pixelFormat)
{
    switch (pixelFormat) {
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR10P:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR12P:
        cv::cvtColor(mat, mat, cv::COLOR_BayerBG2RGB);
        break;
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGBRG10P:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGBRG12P:
        cv::cvtColor(mat, mat, cv::COLOR_BayerGB2RGB);
        break;
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SGRBG10P:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SGRBG12P:
        cv::cvtColor(mat, mat, cv::COLOR_BayerGR2RGB);
        break;
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SRGGB10P:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SRGGB12P:
        cv::cvtColor(mat, mat, cv::COLOR_BayerRG2RGB);
        break;
    }
}

void ImageProcessor::ResizeImage(cv::UMat &image, const ResizeOptions &options)
{
    cv::UMat inputImage = image;
    cv::resize(inputImage, image, cv::Size(options.width, options.height));
}

void ImageProcessor::DrawCrosshair(cv::UMat &mat)
{
    cv::Mat image = mat.getMat(cv::ACCESS_READ);

    cv::drawMarker(image, cv::Point(mat.cols/2, mat.rows/2), cv::Scalar(0, 0, 255));
}

void ImageProcessor::DrawFps(cv::UMat &mat, uint32_t acquisitionFps, uint32_t displayFps)
{
    std::string fpsText =  std::to_string(acquisitionFps) + "/" + std::to_string(displayFps) + " fps";
    int fpsFont = cv::FONT_HERSHEY_SIMPLEX;
    double fpsScale = 0.7;
    int fpsThickness = 1;
    cv::Point fpsPoint(10, 30);

    uint32_t grayBackground;
    uint32_t grayText;
    if (mat.depth() == CV_8U) {
        grayBackground = 0x20;
        grayText = 0xd0;
    } else if (mat.depth() == CV_16U) {
        grayBackground = 0x2000;
        grayText = 0xd000;
    } else {
        throw std::invalid_argument("Unsupported bit depth");
    }

    int fpsBaseline;

    cv::Size fpsSize = cv::getTextSize(fpsText, fpsFont, fpsScale, fpsThickness, &fpsBaseline);

    cv::rectangle(
        mat,
        fpsPoint + cv::Point(0, fpsBaseline),
        fpsPoint + cv::Point(fpsSize.width, -fpsSize.height - 5),
        cv::Scalar(grayBackground, grayBackground, grayBackground),
        cv::FILLED
    );

    cv::putText(
        mat, 
        fpsText,
        fpsPoint,
        fpsFont,
        fpsScale,
        cv::Scalar(grayText, grayText, grayText),
        fpsThickness
    );
}

}
