#include "image_writer.hpp"
#include "image_util.hpp"
#include <fstream>
#include <opencv2/imgcodecs/imgcodecs.hpp>

namespace common
{

void ImageWriter::SaveImageAsPng(cv::UMat &mat, std::string name)
{
    cv::imwrite(GetAvailableName(name, ".png"), mat);
}

void ImageWriter::SaveImageAsJpeg(cv::UMat &mat, std::string name)
{
    if (mat.depth() != CV_8U)
        ConvertTo8Bit(mat);

    cv::imwrite(GetAvailableName(name, ".jpg"), mat);
}

void ImageWriter::SaveImageAsTiff(cv::UMat &mat, std::string name)
{
    cv::imwrite(GetAvailableName(name, ".tiff"), mat);
}

std::string ImageWriter::GetAvailableName(std::string firstPart, std::string secondPart)
{
    uint32_t index = 0;

    std::string file = firstPart + "_" + std::to_string(index) + secondPart;
    while(FileExists(file)) {
        index++;
        file = firstPart + "_" +  std::to_string(index) + secondPart;
    }

    return file;
}

bool ImageWriter::FileExists(std::string file)
{
    return std::ifstream(file).good();
}

}