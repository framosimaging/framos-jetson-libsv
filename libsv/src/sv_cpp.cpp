#include "sv/sv.h"
#include "globals.hpp"
#include "camera.hpp"
#include "image_processing.hpp"
#include "easylogging++/easylogging++.h"

namespace sv
{
    ICameraList GetAllCameras()
    {
        auto cameraList = global::Global::GetCameras();
        ICameraList icameraList;

        for(auto& camera : cameraList) {
            icameraList.push_back(dynamic_cast<ICamera*>(camera));
        }

        return icameraList;
    }

    ICamera* GetCamera(const char* node)
    {   
        auto cameraList = global::Global::GetCameras();

        for(auto& camera : cameraList) {
            if(camera->GetName() == node) {
                return dynamic_cast<ICamera*>(camera);
            }
        }
        return nullptr;
    }

    IProcessedImage AllocateProcessedImage(IImageInfo imageInfo)
    {
        IProcessedImage processedImage = {};
        processedImage.width = imageInfo.width;
        processedImage.height = imageInfo.height;
        processedImage.pixelFormat = imageInfo.pixelFormat;

        uint32_t bpp = v4l2::format::GetBpp(imageInfo.pixelFormat);
        switch(bpp) {
        case 8:
            processedImage.length = imageInfo.width * imageInfo.height;
            break;
        case 10:
        case 12:
            processedImage.length = imageInfo.width * imageInfo.height * 2;
            break;
        default:
            LOG(ERROR) << "Unable to allocate processing buffers with unsupported bit depth: " << bpp;
            return processedImage;
        }

        processedImage.stride = processedImage.length / processedImage.height;

        processedImage.data = new uint8_t [processedImage.length];
        processedImage.embeddedData = new uint8_t [EMBEDDED_DATA_MAX_SIZE];

        return processedImage;
    }

    bool ProcessImage(const IImage &input, IProcessedImage &output, SV_PLATFORM_PROCESSING algorithm)
    {
        if (input.data == nullptr) {
            LOG(ERROR) << "Processing input data is null!";
            return false;
        }

        if (output.data == nullptr) {
            LOG(ERROR) << "Processing output data is null!";
            return false;
        }

        if (input.width != output.width || input.height != output.height || input.pixelFormat != output.pixelFormat) {
            LOG(ERROR) << "Processing buffer mismatch! Input: " << input.width << "x" << input.height << "(" << input.pixelFormat << ")" 
                << ", output: " << output.width << "x" << output.height << "(" << output.pixelFormat << ")";
            return false;
        }

        switch (algorithm) {
        case SV_ALGORITHM_AUTODETECT:
            return processing::ProcessAutodetect(input, output);
        default:
            LOG(ERROR) << "Unknown processing algorithm: " << algorithm;
            return false;
        }
    }

    void DeallocateProcessedImage(IProcessedImage &image)
    {
        delete [] (uint8_t *)image.data;
        delete [] (uint8_t *)image.embeddedData;
        image = {};
    }

    SV_PLATFORM GetPlatform()
    {
        return v4l2::platform;
    }
}


ICamera::~ICamera()
{

}


IControl::~IControl()
{
    
}