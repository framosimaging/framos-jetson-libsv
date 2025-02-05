#pragma once

#include "sv/sv.h"
#include "resize_options.hpp"
#include "configured_camera.hpp"
#include "debayering_control.hpp"
#include "resize_control.hpp"
#include <string>
#include <memory>

namespace common 
{
    class FrameSize;

    class CameraConfigurator {
        public:
            CameraConfigurator(ICameraList cameras);
            ~CameraConfigurator();
            ConfiguredCameras Configure();
        private:
            ICameraList cameras;
            DebayeringControl debayeringControl;
            ResizeControl resizeControl;
            void ConfigureControls(ConfiguredCamera &configuredCamera);
            void ConfigureControl(IControl *control, std::string camera);
            void ConfigureMenu(IControl *control, std::string camera);
            void ConfigureInteger(IControl *control, std::string camera);
            void ConfigureProcessing();
            bool GetConfigurationMenuEntry(int32_t &index);
            bool GetCameraEntry(ICamera *&camera);
            bool GetControlEntry(ICamera *camera, IControl *&control);
            std::string GetCurrentControlValue(IControl *control);
    };
}