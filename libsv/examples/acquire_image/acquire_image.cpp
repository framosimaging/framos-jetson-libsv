#include "sv/sv.h"

#include "common_cpp/common.hpp"

#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> streamActive;
std::atomic<uint32_t> framesCaptured;

void MeasureFps() ;
void CaptureImage(ICamera *camera, bool processing);

int main() 
{
    // Use sv::GetAllCameras() to get a list of available cameras
    ICameraList cameras = sv::GetAllCameras();

    if (cameras.size() == 0) {
        std::cout << "No cameras detected! Exiting..." << std::endl;
        return 0;
    }

    // Select one of available cameras 
    ICamera *camera = common::SelectCamera(cameras);

    // Use GetControl() to select control by id
    IControl *control = camera->GetControl(SV_API_FETCHBLOCKING);

    // Check if control is available before using it
    if (control) {

        // Use GetDefaultValue() to obtain default value
        int32_t defaultValue = control->GetDefaultValue();

        // Use Set() to set the control value
        control->Set(defaultValue);
    }
    
    // Use SV_V4L2_IMAGEFORMAT to set image format
    control = camera->GetControl(SV_V4L2_IMAGEFORMAT);
    if (control) {
        common::SelectPixelFormat(control);
    }

    // Use SV_V4L2_FRAMESIZE to set resolution
    common::SelectFrameSize(camera->GetControl(SV_V4L2_FRAMESIZE));

    // Use GetControlList() to get a list of available controls
    IControlList controls = camera->GetControlList();

    // Set other controls matched by name
    std::set<std::string> queryControls = {"Gain", "Digital Gain", "Exposure", "Coarse Time", "Frame Length", "Frame Rate"};
    for (IControl *control : controls) {
        std::string name = std::string(control->GetName());
        if (queryControls.find(name) != queryControls.end()) {
            int32_t value = common::SelectValue(name, control->GetMinValue(), control->GetMaxValue(), control->GetDefaultValue());
            control->Set(value);
        }
    }

    bool processing = common::SelectEnable("platform-specific processing", false);

    // Use StartStream() to start acquiring frames
    if (!camera->StartStream()) {
        std::cout << "Failed to start stream!" << std::endl;
        return 0;
    }
    streamActive = true;

    std::thread captureThread(CaptureImage, camera, processing);
    std::thread fpsThread(MeasureFps);
    
    common::ExitOnEnter();

    streamActive = false;
    
    if (fpsThread.joinable()) {
        fpsThread.join();
    }

    if (captureThread.joinable()) {
        captureThread.join();
    }

    // Use StopStream() to stop acquiring frames
    camera->StopStream();

    return 0;
}

void MeasureFps() 
{
    while (streamActive) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (streamActive) {
            std::cout << " " << framesCaptured << "fps" << std::endl;
            framesCaptured = 0;    
        }        
    }
}

void CaptureImage(ICamera *camera, bool processing)
{
    framesCaptured = 0;

    // Allocate processing buffers
    IProcessedImage processedImage = sv::AllocateProcessedImage(camera->GetImageInfo());

    while (streamActive) {

        // Get a frame using GetImage()
        IImage image = camera->GetImage();

        // Check if frame is valid before using it
        if (image.data == nullptr) {
            continue;
        }

        if (processing) {
            // Perform platform specific processing
            sv::ProcessImage(image, processedImage, SV_ALGORITHM_AUTODETECT);
        }

        std::cout << "." << std::flush;
        framesCaptured += 1;

        // Release a frame after it is no longer needed
        camera->ReturnImage(image);
    }
        
    // Deallocate processing buffer
    sv::DeallocateProcessedImage(processedImage);
}
