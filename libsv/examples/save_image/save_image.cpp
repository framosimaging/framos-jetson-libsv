#include "sv/sv.h"
#include "common_cpp/common.hpp"

#include <linux/limits.h>

std::string GetCurrentWorkingDir();
void SaveFrame(void *data, uint32_t length, std::string name, std::string folder);

int main() 
{
    ICameraList cameras = sv::GetAllCameras();
    if (cameras.size() == 0) {
        std::cout << "No cameras detected! Exiting..." << std::endl;
        return 0;
    }

    ICamera *camera = common::SelectCamera(cameras);

    IControl *control = camera->GetControl(SV_V4L2_IMAGEFORMAT);
    if (control) {
        common::SelectPixelFormat(control);
    }

    IControlList controls = camera->GetControlList();

    common::SelectFrameSize(camera->GetControl(SV_V4L2_FRAMESIZE));

    std::set<std::string> queryControls = {"Gain", "Digital Gain", "Exposure", "Coarse Time", "Frame Length", "Black Level"};
    for (IControl *control : controls) {
        std::string name = std::string(control->GetName());
        if (queryControls.find(name) != queryControls.end()) {
            int32_t value = common::SelectValue(name, control->GetMinValue(), control->GetMaxValue(), control->GetDefaultValue());
            control->Set(value);
        }
    }
    
    bool processing = common::SelectEnable("platform-specific processing", control->GetDefaultValue());

    bool saveEmbeddedData = common::SelectEnable("save embedded data", control->GetDefaultValue());

    if (!camera->StartStream()) {
        std::cout << "Failed to start stream!" << std::endl;
        return 0;
    }

    const int minFrameNumber = 1;
    const int maxFrameNumber = 1000;
    const int defFrameNumber = 10;
    const int frameCount = common::SelectValue("Numbers of frames you wish to save", minFrameNumber, maxFrameNumber, defFrameNumber);
    const std::string folder = GetCurrentWorkingDir() + "/output/";

    IProcessedImage processedImage = sv::AllocateProcessedImage(camera->GetImageInfo());

    int frameSaved = 0;
    for (int i = 0; i < frameCount; i++) {
        
        IImage image = camera->GetImage();

        if (image.data == nullptr) {
            std::cout << "Unable to save frame, invalid image data" << std::endl;
            continue;
        }

        void *data;
        uint32_t length;
        if (processing) {
            sv::ProcessImage(image, processedImage, SV_ALGORITHM_AUTODETECT);
            data = processedImage.data;
            length = processedImage.length;
        } else {
            data = image.data;
            length = image.length;
            if (data == nullptr) {
                std::cout << "data null" << std::endl;
            }
        }

        void *embeddedData = nullptr;
        uint32_t lengthEmbeddedData = 0;
        if (saveEmbeddedData) {
            if (processing) {
                embeddedData = processedImage.embeddedData;
                lengthEmbeddedData = processedImage.embeddedDataWidth * processedImage.embeddedDataHeight;
            } else {
                embeddedData = image.embeddedData;
                lengthEmbeddedData = image.embeddedDataWidth * image.embeddedDataHeight * 2;
            }
        }

        SaveFrame(data, length, "frame" + std::to_string(i) + ".raw", folder);

        if (saveEmbeddedData) {
            if (lengthEmbeddedData != 0)
                SaveFrame(embeddedData, lengthEmbeddedData, "embedded" + std::to_string(i) + ".raw", folder);
            else
                std::cout << "embedded data length is zero" << std::endl;
        }

        std::cout << "." << std::flush;
        frameSaved += 1;

        camera->ReturnImage(image);
    }

    camera->StopStream();

    std::cout << "\nSaved " << frameSaved << " frames to " << folder << " folder." << std::endl;

    sv::DeallocateProcessedImage(processedImage);

    return 0;
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

void SaveFrame(void *data, uint32_t length, std::string name, std::string folder)
{
    mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    std::string file = folder + name;
    remove(file.c_str());
    int fd = open (file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        std::cout << "Unable to save frame, cannot open file " << file << std::endl;
        return;
    }
        
    int writenBytes = write(fd, data, length); 
    if (writenBytes < 0) {
        std::cout << "Error writing to file " << file << std::endl;
    } else if ( (uint32_t) writenBytes != length) {
        std::cout << "Warning: " << writenBytes << " out of " << length << " were written to file " << file << std::endl;
    }

    close(fd);
}
