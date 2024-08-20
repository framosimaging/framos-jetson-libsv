#include <atomic>
#include <iostream>
#include "v4l2isp/v4l2isp.hpp"
#include "v4l2isp/common.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

std::atomic<bool> streamActive;


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
    std::cout << "Press any key to EXIT\n";
    
	while(streamActive){
	
		cv::UMat bgr;
		auto image = camera->GetImage();
		if(image->planeY == nullptr || image->planeUV == nullptr) {
			continue;
		}
		
		cv::Mat y(camera->GetHeight(), camera->GetWidth(), CV_8UC1, image->planeY, image->strideY);
		cv::Mat uv(camera->GetHeight() / 2, camera->GetWidth() / 2, CV_8UC2, image->planeUV, image->strideUV); 
		
		cv::UMat yUMat = y.getUMat(cv::ACCESS_READ);
		cv::UMat uvUMat = uv.getUMat(cv::ACCESS_READ);
		cv::cvtColorTwoPlane(yUMat, uvUMat, bgr, cv::COLOR_YUV2BGR_NV12);
		cv::namedWindow(camera->GetName() + " " + camera->GetDriverName(), cv::WINDOW_OPENGL | cv::WINDOW_AUTOSIZE);
		cv::imshow(camera->GetName() + " " + camera->GetDriverName(), bgr); 	
		
		int key = cv::waitKey(1);		
		if(key >= 0) {
			streamActive = false;
		}
		camera->ReturnImage();	
	}
		
	camera->StopStream();
	
    return EXIT_SUCCESS;
} 
catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
