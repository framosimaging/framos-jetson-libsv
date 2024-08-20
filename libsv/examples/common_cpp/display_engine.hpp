#pragma once

#include "sv/sv.h"
#include "resize_options.hpp"
#include "configured_camera.hpp"
#include "image_writer.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace common
{
    enum class SaveImageOptions { PNG, JPEG, TIFF, DISABLED };

    class HotkeyAction;
    class ImagePipeline;

    class DisplayEngine
    {
        public:
            DisplayEngine(const ConfiguredCameras &cameras);
            DisplayEngine(const ConfiguredCamera &camera);
            ~DisplayEngine();

            void Start();
            
            ResizeOptions GetResizeOptions();
            void SetResizeOptions(ResizeOptions options);
        
        private:

            ResizeOptions resizeOptions;
            
            bool toggleFps, toggleCrosshair;
            SaveImageOptions saveImage;
            std::vector<HotkeyAction> hotkeyActions;

            std::vector<std::unique_ptr<ImagePipeline>> imagePipelines;

            ImageWriter imageWriter;

            std::atomic<bool> streamActive;

            void Construct(const ConfiguredCameras &cameras);

            void ConstructWindows();

            void ConstructHotkeyActions();
            void DisplayHotkeyActions();
            void PerformHotkeyActions(int hotkey);
            
            void StartImagePipelines();
            void StopImagePipelines();

            void SaveImage(cv::UMat &image, std::string name);
    };
}