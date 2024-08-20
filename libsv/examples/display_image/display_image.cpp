#include "sv/sv.h"

#include "common_cpp/common.hpp"
#include "common_cpp/display_engine.hpp"
#include "common_cpp/camera_configurator.hpp"

#include <memory>

int main() 
{
    ICameraList cameras = sv::GetAllCameras();
    if (cameras.size() == 0) {
        std::cout << "No cameras detected! Exiting..." << std::endl;
        return 0;
    }

    auto selected = common::SelectMultipleCameras(cameras);

    common::ConfiguredCameras configuredCameras = common::CameraConfigurator(selected).Configure();

    common::DisplayEngine(configuredCameras).Start();

    return 0;
}

