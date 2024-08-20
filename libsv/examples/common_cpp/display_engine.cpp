#include "display_engine.hpp"

#include "hotkey_action.hpp"
#include "sequential_image_pipeline.hpp"
#include "parallel_image_pipeline.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <opencv2/highgui.hpp>

namespace common
{

DisplayEngine::DisplayEngine(const ConfiguredCameras &cameras)
{
    Construct(cameras);
}

DisplayEngine::DisplayEngine(const ConfiguredCamera &camera)
{
    ConfiguredCameras cameras;
    cameras.push_back(camera);
    Construct(cameras);
}

DisplayEngine::~DisplayEngine()
{
    
}

void DisplayEngine::Construct(const ConfiguredCameras &cameras)
{
    if (cameras.empty()) {
        throw std::invalid_argument("Camera list empty"); 
    }

    resizeOptions = {};

    toggleFps = false;
    toggleCrosshair = false;
    saveImage = SaveImageOptions::DISABLED;
    ConstructHotkeyActions();

    for (auto camera : cameras) {
        if (sv::GetPlatform() == SV_PLATFORM_DRAGONBOARD_410C)
            imagePipelines.push_back(std::unique_ptr<ImagePipeline>(new SequentialImagePipeline(camera.camera)));
        else
            imagePipelines.push_back(std::unique_ptr<ImagePipeline>(new ParallelImagePipeline(camera.camera)));
        imagePipelines.back()->SetDebayer(camera.debayering);
        imagePipelines.back()->SetResizeOptions(camera.resizeOptions);
    }
}

void DisplayEngine::ConstructWindows()
{
    for (auto &pipeline : imagePipelines) {
        cv::namedWindow(pipeline->GetName(), cv::WINDOW_OPENGL | cv::WINDOW_AUTOSIZE);
    }
}

void DisplayEngine::ConstructHotkeyActions()
{
    hotkeyActions.emplace_back(HotkeyAction(
        {'f', 'F'},
        "Toggle FPS counter",
        [this](){ toggleFps = true; }
    ));

    hotkeyActions.emplace_back(HotkeyAction(
        {'c', 'C'},
        "Toggle Crosshair overlay",
        [this](){ toggleCrosshair = true; }
    ));

    hotkeyActions.emplace_back(HotkeyAction(
        {'p', 'P'},
        "Save snapshot in PNG format",
        [this](){ saveImage = SaveImageOptions::PNG; }
    ));

    hotkeyActions.emplace_back(HotkeyAction(
        {'j', 'J'},
        "Save snapshot in JPEG format",
        [this](){ saveImage = SaveImageOptions::JPEG; }
    ));

    hotkeyActions.emplace_back(HotkeyAction(
        {'t', 'T'},
        "Save snapshot in TIFF format",
        [this](){ saveImage = SaveImageOptions::TIFF; }
    ));

    hotkeyActions.emplace_back(HotkeyAction(
        {'q', 'Q', SpecialHotkey::ENTER, SpecialHotkey::ENTER_NUM, SpecialHotkey::ESC},
        "Exit application",
        [this](){ streamActive = false; }
    ));
}

/**
 * Streams from multiple sensors are displayed in separate windows. Windows are refreshed sequentially 
 * because OpenCV functions manipulating GUI (imshow, waitKey) must be called from the main thread.
 * This may limit display fps when streaming from multiple sensors.
 */
void DisplayEngine::Start()
{   
    StartImagePipelines();
    DisplayHotkeyActions();
    ConstructWindows();

    streamActive = true;
    while (streamActive) {

        for (auto &pipeline : imagePipelines) {
        
            cv::UMat image = pipeline->GetImage();
            cv::imshow(pipeline->GetName(), image);

            if (toggleCrosshair)
                pipeline->ToggleCrosshair();

            if (toggleFps)
                pipeline->ToggleShowFps();

            if (saveImage != SaveImageOptions::DISABLED)
                SaveImage(image, pipeline->GetCleanName());

            pipeline->ReturnImage();
        }

        saveImage = SaveImageOptions::DISABLED;
        toggleFps = false;
        toggleCrosshair = false;

        int key = cv::waitKey(1);
        if (key >= 0) {
            PerformHotkeyActions(key);
        }
    }

    StopImagePipelines();
}

/**
 * Sensors in master mode have to be started before sensors in slave mode that they control.
 * Application may hang otherwise.
 */
void DisplayEngine::StartImagePipelines()
{
    std::for_each(imagePipelines.begin(), imagePipelines.end(), 
        [](std::unique_ptr<ImagePipeline> &pipeline) { 
            if (pipeline->IsMaster()) pipeline->Start(); 
        }
    );

    std::for_each(imagePipelines.begin(), imagePipelines.end(),
        [](std::unique_ptr<ImagePipeline> &pipeline) {
            if (!pipeline->IsMaster()) pipeline->Start();
        }
    );
}

/**
 * Sensors in master mode have to be stopped after sensors in slave mode that they control.
 * Application may hang otherwise.
 */
void DisplayEngine::StopImagePipelines()
{
    std::for_each(imagePipelines.begin(), imagePipelines.end(),
        [](std::unique_ptr<ImagePipeline> &pipeline) {
            if (!pipeline->IsMaster()) pipeline->Stop();
        }
    );

    std::for_each(imagePipelines.begin(), imagePipelines.end(), 
        [](std::unique_ptr<ImagePipeline> &pipeline) { 
            if (pipeline->IsMaster()) pipeline->Stop(); 
        }
    );
}

void DisplayEngine::DisplayHotkeyActions()
{
    std::cout << std::endl << "Available hotkeys:" << std::endl;
    for (auto const &hotkeyAction : hotkeyActions) {
        std::cout << hotkeyAction.GetDescription() << std::endl;
    }
}

void DisplayEngine::PerformHotkeyActions(int hotkey)
{
    for (auto const &hotkeyAction : hotkeyActions) {
        if (hotkeyAction.IsValidHotkey(hotkey)) {
            hotkeyAction.Perform();
            return;
        }
    }
}

void DisplayEngine::SaveImage(cv::UMat &image, std::string name)
{
    switch(saveImage) {
        case SaveImageOptions::JPEG:
            imageWriter.SaveImageAsJpeg(image, name);
            break;
        case SaveImageOptions::PNG:
            imageWriter.SaveImageAsPng(image, name);
            break;
        case SaveImageOptions::TIFF:
            imageWriter.SaveImageAsTiff(image, name);
            break;
        default:
            break;
    }
}

}
