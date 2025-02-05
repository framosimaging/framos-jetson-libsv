#include <fstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>
#include "v4l2isp/v4l2isp.hpp"
#include "v4l2isp/common.hpp"


std::string GetCurrentWorkingDir();

bool SaveFrame(const v4l2isp::Image *image, uint32_t width, uint32_t height, std::string name, std::string folder);


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
	camera->SetFrameRate(30);
	
	
	const int minFrameNumber = 1;
    const int maxFrameNumber = 1000;
    const int defFrameNumber = 10;
    const int frameCount = v4l2isp::SelectValue("Numbers of frames you wish to save", minFrameNumber, maxFrameNumber, defFrameNumber);
    const std::string folder = GetCurrentWorkingDir() + "/output/";
    int frameSaved = 0;
	
    camera->StartStream();

    for(int i = 0; i < frameCount; i++){
		auto image = camera->GetImage();
		if(image->planeY == nullptr || image->planeUV == nullptr) {
			std::cout << "Unable to save frame, invalid image data" << std::endl;
			continue;
		}
		
		
		if(SaveFrame(image, frameSize.width, frameSize.height, "frame" + std::to_string(i) + ".raw", folder)){
			frameSaved += 1;
		}
		
		std::cout << "." <<  std::flush;
          
        
        camera->ReturnImage();
	}	
    camera->StopStream();

    std::cout << "\nSaved " << frameSaved << " frames to " << folder << " folder." << std::endl;
    
    return EXIT_SUCCESS;
} 
catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}

std::string GetCurrentWorkingDir()
{
    char buff[PATH_MAX];
    if (getcwd(buff, PATH_MAX) == NULL) {
        return "";
    }

    std::string current_working_dir(buff);
    return current_working_dir;
}


bool SaveFrame(const v4l2isp::Image *image, uint32_t width, uint32_t height, std::string name, std::string folder) 
{
	
	mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	std::string file = folder + name;
    remove(file.c_str());
	int fd = open (file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        std::cout << "Unable to save frame, cannot open file " << file << std::endl;
        return false;
    }
    
    uint32_t length = width * height * 1.5;
    int writenBytes = 0; 
    
    for(uint32_t j = 0; j < height; j++) {
		writenBytes +=  write(fd, image->planeY + j * image->strideY, width);
	}
    
    for(uint32_t j = 0; j < height / 2 ; j++) {
		writenBytes += write(fd, image->planeUV + j * image->strideUV, width);
	}
    
    if (writenBytes == 0) {
        std::cout << "Error writing to file " << file << std::endl;
    } else if ( (uint32_t) writenBytes != length) {
        std::cout << "Warning: " << writenBytes << " out of " << length << " were written to file " << file << std::endl;
    }
    
    return true;
}



