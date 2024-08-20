/**
 * OpenCV support for C language is deprecated and will be removed in a future release of OpenCV library.
 * Developing new OpenCV applications in C is not recommended.
 */

#include "display_image.h"
#include "common_c/process_image.h"
#include "common_c/common.h"
#include <opencv2/highgui/highgui_c.h>
#include <pthread.h>
#include <sys/queue.h>

const char WINDOW_NAME[] = "Display Image Example";
const int ASCII_ENTER = 13;
const int ASCII_ESC = 27;

DisplayOptions displayOptions;

bool streamActive = false;

uint32_t framesCaptured = 0;
uint32_t framesDisplayed = 0;

IImage capturedImage;
bool capturedImageAvailable = false;
pthread_mutex_t captureMutex;
pthread_cond_t captureCondition;

CvMat *cvProcessingImage = NULL;
CvMat *cvProcessedImage = NULL;
bool cvProcessedImageAvailable = false;
pthread_mutex_t processMutex;
pthread_cond_t processCondition;

pthread_mutex_t returnMutex;

typedef struct ImageEntry {    
    SLIST_ENTRY(ImageEntry) imagePtr;
    IImage image;
} ImageEntry;

SLIST_HEAD(slisthead, ImageEntry) returnQueue = SLIST_HEAD_INITIALIZER(returnQueue);


void ReturnImages(ICamera *camera)
{
    ImageEntry *img;
    
    pthread_mutex_lock(&returnMutex);
    
    while (!SLIST_EMPTY(&returnQueue)) {
        img = SLIST_FIRST(&returnQueue);
        sv_camera_ReturnImage(camera, img->image);
        SLIST_REMOVE_HEAD(&returnQueue, imagePtr);
        free(img);
    }
    
    pthread_mutex_unlock(&returnMutex);
}

void AddToReturnQueue(IImage *image)
{
    ImageEntry *newImg;
    
    newImg = malloc(sizeof(ImageEntry));
    newImg->image = *image;
    
    pthread_mutex_lock(&returnMutex);

    SLIST_INSERT_HEAD(&returnQueue, newImg, imagePtr);
    
    pthread_mutex_unlock(&returnMutex);
}

void SetCapturedImage(IImage *image)
{
    pthread_mutex_lock(&captureMutex);

    if (capturedImageAvailable) {
        AddToReturnQueue(&capturedImage);
    }

    capturedImage = *image;
    capturedImageAvailable = true;
    
    pthread_mutex_unlock(&captureMutex);
    pthread_cond_broadcast(&captureCondition);
}

void GetCapturedImage(IImage *image)
{
    pthread_mutex_lock(&captureMutex);
    while (!capturedImageAvailable && streamActive) 
        pthread_cond_wait(&captureCondition, &captureMutex);

    *image = capturedImage;
    capturedImageAvailable = false;
    pthread_mutex_unlock(&captureMutex);
}

void SetProcessedImage(CvMat *image)
{
    pthread_mutex_lock(&processMutex);

    if (cvProcessedImageAvailable) {
        DestroyProcessedImageOutput(cvProcessedImage);
    }

    cvProcessedImage = image;
    cvProcessedImageAvailable = true;

    pthread_mutex_unlock(&processMutex);
    pthread_cond_broadcast(&processCondition);
}

void GetProcessedImage(CvMat **image)
{
    pthread_mutex_lock(&processMutex);
    while (!cvProcessedImageAvailable && streamActive) 
        pthread_cond_wait(&processCondition, &processMutex);

    *image = cvProcessedImage;
    cvProcessedImage = NULL;
    cvProcessedImageAvailable = false;
    pthread_mutex_unlock(&processMutex);
}

void *CaptureImageThread(void *camera)
{
    ICamera *cam = (ICamera*) camera;
    
    while (streamActive) {

        IImage image = sv_camera_GetImage(cam);

        if (image.data == NULL) {
            continue;
        }

        ++framesCaptured;

        SetCapturedImage(&image);

        ReturnImages(cam);
    }
    return NULL;
}

void *ProcessImageThread(void *cameraData)
{
    ICamera *camera = (ICamera*)cameraData;

    IProcessedImage svProcessedImage = sv_AllocateProcessedImage(sv_camera_GetImageInfo(camera));
    CvMat *cvProcessingImage;
    IImage capturedImage;

    while (streamActive) {

        GetCapturedImage(&capturedImage);

        sv_ProcessImage(&capturedImage, &svProcessedImage, SV_ALGORITHM_AUTODETECT);
        
        sv_camera_ReturnImage(camera, capturedImage);

        CvProcessImage(&svProcessedImage, &cvProcessingImage, &displayOptions);
        
        SetProcessedImage(cvProcessingImage);
    }

    sv_DeallocateProcessedImage(&svProcessedImage);

    return NULL;
}

void DisplayImage()
{
    CvMat *processedImage;

    while(streamActive) {

        GetProcessedImage(&processedImage);

        cvShowImage(WINDOW_NAME, processedImage);
        int key = cvWaitKey(1);
        
        if (key == ASCII_ENTER || key == ASCII_ESC || key == 'q' || key == 'Q') {
            streamActive = false;
            pthread_cond_broadcast(&captureCondition);
            pthread_cond_broadcast(&processCondition);
        }

        ++framesDisplayed;
        
        DestroyProcessedImageOutput(processedImage);
    }
}

void *MeasureFpsThread() 
{
    while(streamActive) {
        
        sleep(1);
        
        displayOptions.acquisitionFps = framesCaptured;
        displayOptions.displayFps = framesDisplayed;
        
        framesCaptured = 0;
        framesDisplayed = 0;
    }
    return NULL;
}

bool GetCurrentFrameSize(ICamera *camera, FrameSize *frameSize)
{
    IImageInfo info = sv_camera_GetImageInfo(camera);
    frameSize->width = info.width;
    frameSize->height = info.height;

    return true;
}

bool IsFrameSizeSmaller(FrameSize size, FrameSize otherSize)
{
    if (size.width < otherSize.width && size.height < otherSize.height)
        return true;
    
    if (size.width < otherSize.width && size.height == otherSize.height)
        return true;
    
    if (size.width == otherSize.width && size.height < otherSize.height)
        return true;
    
    return false;
}

enum ResizeValue SelectFromResizeMenu(ResizeEntry resizeEntries[], uint32_t resizeEntriesSize, enum ResizeValue defaultResizeValue)
{
    char selectionVector[8][64];
    enum ResizeValue returnVector[8];
    uint32_t vectorIndex = 0;
    
    ResizeEntry resizeEntry;
    
    uint32_t selectionIndex;
    
    uint32_t i;
    
    if (defaultResizeValue == None) {
        sprintf(selectionVector[vectorIndex], "None");
        returnVector[vectorIndex] = None;
        ++vectorIndex;
    }
    
    for (i = 0; i < resizeEntriesSize; i++) {
        resizeEntry = resizeEntries[i];
        sprintf(selectionVector[vectorIndex], "%dx%d", resizeEntry.frameSize.width, resizeEntry.frameSize.height);
        returnVector[vectorIndex] = resizeEntry.resizeValue;
        ++vectorIndex;
    }
    
    if (defaultResizeValue != None) {
        sprintf(selectionVector[vectorIndex], "None");
        returnVector[vectorIndex] = None;
        ++vectorIndex;
    }
    
    selectionIndex = SelectFromMenu("software image resizing resolution", selectionVector, vectorIndex, 0);
    
    return returnVector[selectionIndex];
}

struct ResizeOptions SelectResizeOptions(FrameSize frameSize)
{  
    enum ResizeValue defaultResizeValue = None;
   
    FrameSize fullHdFrameSize = { 1920, 1080 };
    FrameSize hdFrameSize = { 1280, 720 };
    FrameSize vgaFrameSize = { 640, 480 };
    
    ResizeEntry fullHdResizeEntry = { FullHd , fullHdFrameSize };
    ResizeEntry hdResizeEntry = { Hd , hdFrameSize };
    ResizeEntry vgaResizeEntry = { Vga , vgaFrameSize };
    
    enum ResizeValue selectedResizeValue;
    ResizeEntry selectedEntry;
    bool selectionFound = false;
    
    ResizeEntry resizeEntries[8];
    uint32_t resizeEntriesIndex = 0;
    uint32_t resizeEntriesLength;
    
    ResizeOptions resizeOptions;
    
    uint32_t i;
    
    if (IsFrameSizeSmaller(fullHdFrameSize, frameSize)) {
        resizeEntries[resizeEntriesIndex++] = fullHdResizeEntry;
        defaultResizeValue = FullHd;
    }
    
    if (IsFrameSizeSmaller(hdFrameSize, frameSize)) {
        resizeEntries[resizeEntriesIndex++] = hdResizeEntry;
    }
    
    if (IsFrameSizeSmaller(vgaFrameSize, frameSize)) {
        resizeEntries[resizeEntriesIndex++] = vgaResizeEntry;
    }
    
    resizeEntriesLength = resizeEntriesIndex;
    
    selectedResizeValue = SelectFromResizeMenu(resizeEntries, resizeEntriesLength, defaultResizeValue);
    
    for (i = 0; i < resizeEntriesLength; i++) {
        if (resizeEntries[i].resizeValue == selectedResizeValue) {
            selectedEntry = resizeEntries[i];
            selectionFound = true;
            break;
        }
    }
 
    if (selectionFound) {
        resizeOptions.width = selectedEntry.frameSize.width;
        resizeOptions.height = selectedEntry.frameSize.height;
        resizeOptions.enable = true;
    } else {
        resizeOptions.enable = false;
    }
    
    return resizeOptions;    
}

int main() 
{
    int numCameras;
    int numControl;
    int i;
    
    CICameraList cameras = sv_GetAllCameras(&numCameras);
    if (numCameras == 0) {
        printf("No cameras detected! Exiting...\n");
        return 0;
    }
    
    ICamera *camera = SelectCamera(&cameras, numCameras);

    IControl *control = sv_camera_GetControl(camera, SV_V4L2_IMAGEFORMAT);
    if (control) {
        SelectPixelFormat(control);
    }
    
    CIControlList controls = sv_camera_GetControlList(camera, &numControl);

    SelectFrameSize(sv_camera_GetControl(camera, SV_V4L2_FRAMESIZE));

    const char *queryControls[] = {"Gain", "Digital Gain", "Exposure", "Coarse Time", "Frame Length"};
        
    for (i = 0; i < numControl; i++) {
        const char *name = sv_control_GetName((IControl*)controls[i]); 
        if (CheckQueryBool(name, strlen(name), queryControls, ARRAY_LENGTH(queryControls))) {
            int32_t value = SelectValue(name, sv_control_GetMinValue((IControl*)controls[i]), sv_control_GetMaxValue((IControl*)controls[i]), sv_control_GetDefaultValue((IControl*)controls[i]));
            sv_control_Set((IControl*)controls[i], value);
        }
    }
    
    displayOptions.debayer = SelectEnable("software debayering", false);
    
    FrameSize currentFrameSize;
    if (GetCurrentFrameSize(camera, &currentFrameSize)) {
        displayOptions.resizeOptions = SelectResizeOptions(currentFrameSize);
    } else {
        displayOptions.resizeOptions.enable = false;
    }
    
    if (sv_camera_StartStream(camera) == 0) {
        printf("Failed to start stream!\n");
        return 0;
    }
    streamActive = true;

    pthread_t captureThread;
    pthread_t processingThread;
    pthread_t fpsThread;
    
    if (pthread_create(&captureThread, NULL, CaptureImageThread, camera)) {
        printf("Failed to Capture image\n");
        return 0;
    }
    
    if (pthread_create(&processingThread, NULL, ProcessImageThread, camera)) {
        printf("Failed to Process image\n");
        return 0;
    }
    
    if (pthread_create(&fpsThread, NULL, MeasureFpsThread, NULL)) {
        printf("Failed to measure fps\n");
        return 0;
    }
    
    cvNamedWindow(WINDOW_NAME, CV_WINDOW_OPENGL | CV_WINDOW_AUTOSIZE);

    DisplayImage();

    pthread_join(captureThread, NULL);
 
    sv_camera_StopStream(camera);
    
    pthread_join(processingThread, NULL);

    pthread_join(fpsThread, NULL);
    
    free(controls);
    free(cameras);
    DestroyImageProcessingBuffers();

    return 0;
}