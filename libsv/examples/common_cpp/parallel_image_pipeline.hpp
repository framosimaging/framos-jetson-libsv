#pragma once

#include "sv/sv.h"
#include "capture_node.hpp"
#include "sv_processing_node.hpp"
#include "cv_processing_node.hpp"
#include "fps_measurer.hpp"

namespace common
{

    class ParallelImagePipeline : public ImagePipeline
    {
        public:

            explicit ParallelImagePipeline(ICamera *camera) 
            : ImagePipeline(camera), camera(camera)
            {
                captureNode = std::unique_ptr<CaptureNode>(new CaptureNode(camera));
                svProcessingNode = std::unique_ptr<SvProcessingNode>(new SvProcessingNode(*captureNode, camera));
                cvProcessingNode = std::unique_ptr<CvProcessingNode>(new CvProcessingNode(*svProcessingNode, camera));
            }

            ~ParallelImagePipeline()
            {
                
            }

            void Start() override
            {
                captureNode->Start();
                svProcessingNode->Start();
                cvProcessingNode->Start();
            }

            void Stop() override
            {
                cvProcessingNode->Stop();
                svProcessingNode->Stop();
                captureNode->Stop();
            }

            cv::UMat GetImage() override
            {
                return cvProcessingNode->GetOutputBlocking();
            }

            void ReturnImage() override
            {
                cvProcessingNode->ReturnOutput();
                fpsMeasurer.FrameReceived();
                cvProcessingNode->SetFps(captureNode->GetFps(), fpsMeasurer.GetFps());
            }

            void SetDebayer(bool enable) override
            {
                cvProcessingNode->SetDebayer(enable);
            }

            void SetResizeOptions(ResizeOptions resizeOptions) override
            {
                cvProcessingNode->SetResizeOptions(resizeOptions);
            }

            void ToggleCrosshair() override
            {
                cvProcessingNode->ToggleCrosshair();
            }

            void ToggleShowFps() override 
            {
                cvProcessingNode->ToggleShowFps();
            }

            void SetFps(uint32_t acquisitionFps, uint32_t displayFps)
            {
                cvProcessingNode->SetFps(acquisitionFps, displayFps);
            }

        private:
            ICamera *camera;
            std::unique_ptr<CaptureNode> captureNode;
            std::unique_ptr<SvProcessingNode> svProcessingNode;
            std::unique_ptr<CvProcessingNode> cvProcessingNode;
            FpsMeasurer fpsMeasurer;
    };
}