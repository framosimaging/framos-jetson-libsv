#pragma once
#include <opencv2/core/core.hpp>
#include "image_display_controller.hpp"
#include "sv/sv.h"

namespace common
{
    class ImagePipeline : public virtual ImageDisplayController
    {
        public:
            ImagePipeline(ICamera *camera);
            virtual ~ImagePipeline();
            std::string GetName();
            std::string GetCleanName();
            bool IsMaster();
            virtual void Start() = 0;
            virtual void Stop() = 0;
            virtual cv::UMat GetImage() = 0;
            virtual void ReturnImage() = 0;
        
        private:
            ICamera* camera;
            std::string name, cleanName;
    };
}