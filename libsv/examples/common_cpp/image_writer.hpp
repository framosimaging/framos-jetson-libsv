#pragma once

#include <opencv2/core/core.hpp>

namespace common
{
    class ImageWriter
    {
        public:
            void SaveImageAsPng(cv::UMat &mat, std::string name);
            void SaveImageAsJpeg(cv::UMat &mat, std::string name);
            void SaveImageAsTiff(cv::UMat &mat, std::string name);

        private:
            std::string GetAvailableName(std::string firstPart, std::string secondPart);
            bool FileExists(std::string file);
    };
}