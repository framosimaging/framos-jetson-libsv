#pragma once

#include "sv/sv.h"
#include "resize_options.hpp"
#include "image_display_controller.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <atomic>

namespace common
{
    class ImageProcessor : public virtual ImageDisplayController
    {
        public:
            explicit ImageProcessor(uint32_t pixelFormat);
            virtual ~ImageProcessor();
            void SetDebayer(bool enable) override;
            void SetResizeOptions(ResizeOptions resizeOptions) override;
            void ToggleCrosshair() override;
            void ToggleShowFps() override;
            void SetFps(uint32_t acquisitionFps, uint32_t displayFps) override;

        protected:
            void ProcessImage(const IProcessedImage &input, cv::UMat &output);
            void SetPixelFormat(uint32_t pixelFormat);

        private:
            std::atomic<bool> debayer;
            ResizeOptions resizeOptions;
            std::atomic<bool> showCrosshair;
            std::atomic<bool> showFps;
            std::atomic<uint32_t> acquisitionFps, displayFps;
            uint32_t pixelFormat;
            void AllocateMat(const IProcessedImage &input, cv::UMat &output);
            void DebayerImage(cv::UMat &mat, uint32_t pixelFormat);
            void ResizeImage(cv::UMat &image, const ResizeOptions &options);
            void DrawCrosshair(cv::UMat &mat);
            void DrawFps(cv::UMat &mat, uint32_t acquisitionFps, uint32_t displayFps);
            uint8_t GetBpp(uint32_t pixelFormat);
    };
}