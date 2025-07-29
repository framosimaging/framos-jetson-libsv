#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <opencv2/core/core_c.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/hal/interface.h>
#include <linux/videodev2.h>
#include "sv/sv.h"

typedef struct ResizeOptions {
    bool enable;
    uint32_t width;
    uint32_t height;
} ResizeOptions;

typedef struct DisplayOptions {  
    bool debayer;
    ResizeOptions resizeOptions;
    uint32_t acquisitionFps;
    uint32_t displayFps;
} DisplayOptions;

uint8_t GetBpp(uint32_t pixelFormat);

CvMat *ConvertImageTo8Bpp(CvMat *mat);

CvMat *DebayerImage(CvMat *mat, uint32_t pixelFormat);

CvMat *ResizeImage(CvMat *mat, ResizeOptions *options);

void DrawFps(CvArr *mat, uint32_t acquisitionFps, uint32_t displayFps, uint8_t bpp);

void CvProcessImage(const IProcessedImage *input, CvMat **output, DisplayOptions *options);

void DestroyProcessedImageOutput(CvMat *image);

void DestroyImageProcessingBuffers();