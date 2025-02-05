#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include "v4l2isp/v4l2isp.hpp"
#include "v4l2isp/common.hpp"

std::atomic<bool> streamActive;
std::atomic<uint32_t> framesCaptured;

void MeasureFps();
void CaptureImage(std::shared_ptr<v4l2isp::Camera> camera);

int main() try
{
    auto cameras = v4l2isp::GetAllCameras();

    if (cameras.empty()) {
        throw std::invalid_argument("No cameras detected! Exiting...");
    }

    auto camera = v4l2isp::SelectCamera(cameras);
	

    auto pixelformats = camera->GetPixelformats();
    auto pixelformat = v4l2isp::SelectPixelformat(pixelformats);
    camera->SetPixelformat(pixelformat);

    auto frameSize = camera->SelectFrameSize();
    camera->SetFrameSize(frameSize);

    auto controls = camera->GetControls();
    
    for(uint i = 0; i < controls.size(); i++) {
		v4l2isp::SelectNewValue(&controls[i]);
		camera->SetControl(controls[i]);
	}
    
    uint32_t frameRate = v4l2isp::SelectFrameRate();
	camera->SetFrameRate(frameRate);
    

    camera->StartStream();
    streamActive = true;
    std::thread captureThread(CaptureImage, camera);
    std::thread fpsThread(MeasureFps);

    v4l2isp::ExitOnEnter();

    streamActive = false;
    
    if (captureThread.joinable()) {
        captureThread.join();
    }
    if (fpsThread.joinable()) {
        fpsThread.join();
    }

	camera->StopStream();
    return EXIT_SUCCESS;
} 
catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}

void MeasureFps()
{	
    while (streamActive) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << " " << framesCaptured << "fps" << std::endl;
        framesCaptured = 0;               
    }
}

void CaptureImage(std::shared_ptr<v4l2isp::Camera> camera)
{
    while (streamActive) {
        auto image = camera->GetImage();
		if(image->planeY == nullptr || image->planeUV == nullptr) {
			continue;
		}
        std::cout << "." << std::flush;
        framesCaptured += 1;
        camera->ReturnImage();
    }
}
