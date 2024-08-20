#pragma once

#include "node.hpp"
#include "fps_measurer.hpp"
#include "sv/sv.h"

namespace common
{

    class CaptureNode : public Node<IImage>
    {
        public: 

            explicit CaptureNode(ICamera *camera) : camera(camera)
            {

            }

            ~CaptureNode()
            {
                DeintializeAction();
            }

            uint32_t GetFps()
            {
                return fpsMeasurer.GetFps();
            }

            using Node::ReturnOutput;

        protected:

            void PerformAction(IImage &output) override
            {
                output = camera->GetImage();
                fpsMeasurer.FrameReceived();
            }

            void InitializeAction() override
            {
                IControl *control = camera->GetControl(SV_API_FETCHBLOCKING);
                if (control) {
                    control->Set(1);
                } 
                camera->StartStream();
            }

            void DeintializeAction() override
            {
                camera->StopStream();
            }

            void ReturnOutput(IImage &output) override
            {
                camera->ReturnImage(output);
            }

        private:
            ICamera *camera;
            FpsMeasurer fpsMeasurer;
    };
}