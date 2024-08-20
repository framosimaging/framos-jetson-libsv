#include "process_image.h"

#ifndef V4L2_PIX_FMT_SBGGR12P
#define V4L2_PIX_FMT_SBGGR12P v4l2_fourcc('p', 'B', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SGBRG12P
#define V4L2_PIX_FMT_SGBRG12P v4l2_fourcc('p', 'G', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SGRBG12P
#define V4L2_PIX_FMT_SGRBG12P v4l2_fourcc('p', 'g', 'C', 'C')
#endif

#ifndef V4L2_PIX_FMT_SRGGB12P
#define V4L2_PIX_FMT_SRGGB12P v4l2_fourcc('p', 'R', 'C', 'C')
#endif

#define CONVERSION_SCALE_16_TO_8 0.00390625

bool conversionBufferInitialized = false;
CvMat *conversionBuffer;

bool debayeringBufferInitialized = false;
CvMat *debayeringBuffer;

bool resizingBufferInitialized = false;
CvMat *resizingBuffer;

uint8_t GetBpp(uint32_t pixelFormat)
{
    switch (pixelFormat) {
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
        return 8;
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR10P:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGBRG10P:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SGRBG10P:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SRGGB10P:
        return 10;
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR12P:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGBRG12P:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SGRBG12P:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SRGGB12P:
        return 12;
    }

    return 0;
}

CvMat *ConvertImageTo8Bpp(CvMat *mat)
{
    if (!conversionBufferInitialized) {
        conversionBuffer = cvCreateMat(mat->rows, mat->cols, CV_8UC1);
        conversionBufferInitialized = true;
    }
    
    cvConvertScale(mat, conversionBuffer, CONVERSION_SCALE_16_TO_8, 0);
    
    return conversionBuffer;
}

CvMat *DebayerImage(CvMat *mat, uint32_t pixelFormat)
{
    if (!debayeringBufferInitialized) {
        debayeringBuffer = cvCreateMat(mat->rows, mat->cols, CV_8UC3);
        debayeringBufferInitialized = true;
    }
    
    switch (pixelFormat) {
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR10P:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR12P:
        cvCvtColor(mat, debayeringBuffer, CV_BayerBG2RGB);
        break;
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGBRG10P:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGBRG12P:
        cvCvtColor(mat, debayeringBuffer, CV_BayerGB2RGB);
        break;
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SGRBG10P:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SGRBG12P:
        cvCvtColor(mat, debayeringBuffer, CV_BayerGR2RGB);
        break;
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SRGGB10P:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SRGGB12P:
        cvCvtColor(mat, debayeringBuffer, CV_BayerRG2RGB);
        break;
    }
    
    return debayeringBuffer;
}

CvMat *ResizeImage(CvMat *mat, ResizeOptions *options)
{    
    if (!resizingBufferInitialized) {
        resizingBuffer = cvCreateMat(options->height, options->width, mat->type);
        resizingBufferInitialized = true;
    }
    
    cvResize(mat, resizingBuffer, CV_INTER_LINEAR);
    
    return resizingBuffer;
}

void DrawFps(CvArr *mat, uint32_t acquisitionFps, uint32_t displayFps, uint8_t bpp)
{
    char fpsText[256];
    double scale = 0.7;
    int thickness = 1; 
    
    sprintf(fpsText, "%d/%d fps", acquisitionFps, displayFps);
    
    CvFont fpsFont;
    fpsFont.font_face = CV_FONT_HERSHEY_SIMPLEX;
    fpsFont = cvFont(scale, thickness);
    
    CvPoint fpsPoint = cvPoint(10, 30);

    uint32_t grayBackground;
    uint32_t grayText;
    if (bpp == 8) {
        grayBackground = 0x20;
        grayText = 0xd0;
    } else {
        grayBackground = 0x2000;
        grayText = 0xd000;
    }

    int fpsBaseline;

    CvSize fpsSize;
    cvGetTextSize(fpsText, &fpsFont, &fpsSize, &fpsBaseline);
    
    CvPoint partPt1 = cvPoint(0, fpsBaseline);
    CvPoint pt1 = cvPoint((fpsPoint.x + partPt1.x), (fpsPoint.y + partPt1.y));
    CvPoint partPt2 = cvPoint(fpsSize.width, -fpsSize.height - 5);
    CvPoint pt2 = cvPoint((fpsPoint.x + partPt2.x), (fpsPoint.y + partPt2.y));
    
    cvRectangle(
        mat,
        pt1,
        pt2,
        cvScalar(grayBackground, grayBackground, grayBackground, 0),
        CV_FILLED,
        8,
        0
    );

    cvPutText(
        mat, 
        fpsText,
        fpsPoint,
        &fpsFont,
        cvScalar(grayText, grayText, grayText, 0)
    );
}

void CvProcessImage(const IProcessedImage *input, CvMat **output, DisplayOptions *options)
{
    uint8_t bpp = GetBpp(input->pixelFormat);
    
    int type;
    if (bpp == 8) {
        type = CV_8UC1;
    } else {
        type = CV_16UC1;
    }
	
    CvMat inputMat = cvMat(input->height, input->width, type, input->data);
    CvMat *mat = &inputMat;
    
    /**
     * OpenCV function imshow implicitly converts the image bit depth down to 8 bit before displaying the image.
     * This results in data loss that is not visible to the human eye.
     * Converting the image bit depth manually before performing image processing can lead to better performance.
     * Another advantage of this approach is that it facilitates pipelining by separating the tasks of converting
     * the image bit depth from displaying the image.
     *
     * In this case, the image bit depth is manually converted down to 8bit when debayering is enabled. This increases 
     * performance because debayering is an expensive operation and it is faster to perform it on a smaller 8 bit image.
     */
    
    if (bpp > 8 && options->debayer) {
        mat = ConvertImageTo8Bpp(mat);
        bpp = 8;
    }
    
    if (options->debayer) {
        mat = DebayerImage(mat, input->pixelFormat);
    }
    
    if (options->resizeOptions.enable) {
        mat = ResizeImage(mat, &options->resizeOptions);
    }
    
    DrawFps(mat, options->acquisitionFps, options->displayFps, bpp);
    
    (*output) = cvCloneMat(mat);
}

void DestroyProcessedImageOutput(CvMat *image)
{
    cvReleaseMat(&image);
}

void DestroyImageProcessingBuffers()
{
    if (conversionBufferInitialized) 
        cvReleaseMat(&conversionBuffer);
    
    if (debayeringBufferInitialized) 
        cvReleaseMat(&debayeringBuffer);
    
    if (resizingBufferInitialized)
        cvReleaseMat(&resizingBuffer);
}