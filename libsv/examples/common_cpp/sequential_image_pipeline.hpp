#pragma once

#include "sv/sv.h"
#include "capture_node.hpp"
#include "image_pipeline.hpp"
#include "image_processor.hpp"
#include "fps_measurer.hpp"

namespace common
{

    class SequentialImagePipeline : public ImagePipeline, public ImageProcessor
    {
        public:

            explicit SequentialImagePipeline(ICamera *camera) 
            : ImagePipeline(camera), ImageProcessor(camera->GetImageInfo().pixelFormat), camera(camera)
            {
                captureNode = std::unique_ptr<CaptureNode>(new CaptureNode(camera));
                svImage = sv::AllocateProcessedImage(camera->GetImageInfo());
            }

            ~SequentialImagePipeline()
            {
                sv::DeallocateProcessedImage(svImage);
            }

            void Start() override
            {
                captureNode->Start();
            }

            void Stop() override
            {
                captureNode->Stop();
            }

            cv::UMat GetImage() override
            {
                auto rawImage = captureNode->GetOutputBlocking();
                while (rawImage.data == nullptr)
                    rawImage = captureNode->GetOutputBlocking();

                sv::ProcessImage(rawImage, svImage, SV_ALGORITHM_AUTODETECT);
                camera->ReturnImage(rawImage);

                cv::UMat image;
                this->ProcessImage(svImage, image);

                return image;
            }

            void ReturnImage() override
            {
                fpsMeasurer.FrameReceived();
                SetFps(captureNode->GetFps(), fpsMeasurer.GetFps());
            }

        private:
            ICamera *camera;
            std::unique_ptr<CaptureNode> captureNode;
            IProcessedImage svImage;
            FpsMeasurer fpsMeasurer;
    };
}