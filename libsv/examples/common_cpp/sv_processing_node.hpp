#pragma once

#include "node.hpp"
#include "sv/sv.h"
#include "capture_node.hpp"

namespace common
{
    class SvProcessingNode : public Node<IProcessedImage>
    {
        public:

            explicit SvProcessingNode(CaptureNode &captureNode, ICamera *camera) : captureNode(captureNode), camera(camera)
            {

            }

            ~SvProcessingNode()
            {

            }

            using Node::ReturnOutput;

        protected:

            void PerformAction(IProcessedImage &output) override
            {
                IImage image = captureNode.GetOutputBlocking();
                if (image.data != nullptr) {
                    sv::ProcessImage(image, output, SV_ALGORITHM_AUTODETECT);
                }
                captureNode.ReturnOutput();
            }

            void InitializeOutput(IProcessedImage &output) override
            {
                output = sv::AllocateProcessedImage(camera->GetImageInfo());   
            }

            void DeinitializeOutput(IProcessedImage &output) override
            {
                sv::DeallocateProcessedImage(output);
            }

        private:
            CaptureNode &captureNode;
            ICamera *camera;
    };
}