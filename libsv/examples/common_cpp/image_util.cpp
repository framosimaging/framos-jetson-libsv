#include "image_util.hpp"

namespace common
{

void ConvertTo8Bit(cv::UMat &mat)
{
    if (mat.type() != CV_8U) {
        constexpr auto CONVERSION_SCALE_16_TO_8 = 0.00390625;
        mat.convertTo(mat, CV_8U, CONVERSION_SCALE_16_TO_8);
    }
}

}