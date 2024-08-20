#pragma once

#include "node.hpp"
#include "sv_processing_node.hpp"
#include "image_processor.hpp"

namespace common
{
    class CvProcessingNode : public Node<cv::UMat>, public ImageProcessor
    {
        public:

            explicit CvProcessingNode(SvProcessingNode &svProcessingNode, ICamera *camera) 
            : ImageProcessor(camera->GetImageInfo().pixelFormat), svProcessingNode(svProcessingNode), camera(camera)
            {

            }

            ~CvProcessingNode()
            {

            }

            using Node::ReturnOutput;

        protected:

            void InitializeAction() override
            {
                SetPixelFormat(camera->GetImageInfo().pixelFormat);
            }

            void PerformAction(cv::UMat &output) override 
            {
                IProcessedImage image = svProcessingNode.GetOutputBlocking();
                ProcessImage(image, output);
                svProcessingNode.ReturnOutput();
            }

        private:
            SvProcessingNode &svProcessingNode;
            ICamera *camera;
    };
}