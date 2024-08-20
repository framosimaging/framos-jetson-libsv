#include "common_c/common.h"
#include "sv/sv.h"

#include <pthread.h>

bool streamActive = false;
uint32_t framesCaptured = 0;

typedef struct {
    ICamera *camera;
    bool processing;
} CaptureImageData;

void *MeasureFps();
void *CaptureImage(void *captureImageData);

int main() 
{
    int numCameras;
    int numControl;
    bool processing;
    int i;
    
    // Use sv_GetAllCameras(int* size) to get a list of available cameras
    CICameraList cameras = sv_GetAllCameras(&numCameras);
    if (numCameras == 0) {
        printf("No cameras detected! Exiting...\n");
        return 0;
    }
    
    // Select one of available cameras 
    ICamera *camera = SelectCamera(&cameras, numCameras);

    // Use sv_camera_GetControl() to select control by id
    IControl *control = sv_camera_GetControl(camera, SV_API_FETCHBLOCKING);
    
    // Check if control is available before using it
    if (control) {

        // Use GetDefaultValue() to obtain default value
        int32_t defaultValue = sv_control_GetDefaultValue(control);
        
        // Use sv_control_Set() to set the control value
        sv_control_Set(control, defaultValue);
    }

    // Select one of the available image formats
    control = sv_camera_GetControl(camera, SV_V4L2_IMAGEFORMAT);
    if (control) {
        SelectPixelFormat(control);
    }
    
    // Use sv_camera_GetControlList() to get a list of available controls
    CIControlList controls = sv_camera_GetControlList(camera, &numControl);

    // Use SV_V4L2_FRAMESIZE to set resolution
    SelectFrameSize(sv_camera_GetControl(camera, SV_V4L2_FRAMESIZE));

    // Set other controls matched by name
    const char *queryControls[] = {"Gain", "Digital Gain", "Exposure", "Coarse Time", "Frame Length", "Frame Rate"};
    
    for (i = 0; i < numControl; i++) {
        const char *name = sv_control_GetName((IControl*)controls[i]);
        if (CheckQueryBool(name, strlen(name), queryControls, ARRAY_LENGTH(queryControls))) {
            int32_t value = SelectValue(name, sv_control_GetMinValue((IControl*)controls[i]), sv_control_GetMaxValue((IControl*)controls[i]), sv_control_GetDefaultValue((IControl*)controls[i]));
            sv_control_Set((IControl*)controls[i], value);
        }
    }
    
    processing = SelectEnable("platform-specific processing", false);

    // Use sv_camera_StartStream() to start acquiring frames
    if (sv_camera_StartStream(camera) == 0) {
        printf("Failed to start stream!\n");
        return 0;
    }
    streamActive = true;

    pthread_t captureThread;
    pthread_t fpsThread;

    CaptureImageData captureImageData = {camera, processing};
    if (pthread_create(&captureThread, NULL, CaptureImage, &captureImageData)) {
        printf("Failed to acquire image\n");
        return 0;
    }
    
    if (pthread_create(&fpsThread, NULL, MeasureFps, NULL)) {
        printf("Failed to measure fps\n");
        return 0;
    }
    
    WaitForEnter();

    streamActive = false;
    
    pthread_join(fpsThread, NULL);
    pthread_join(captureThread, NULL);
    
    // Use sv_camera_StopStream() to stop acquiring frames
    sv_camera_StopStream(camera);

    free(controls);
    free(cameras);

    return 0;
}

void *MeasureFps()
{
    while (streamActive) {
        sleep(1);

        if (streamActive) {
            printf(" %i fps\n", framesCaptured);
            framesCaptured = 0;
        }
    }
    return NULL;
}

void *CaptureImage(void *captureImageData)
{
    IImage image;

    CaptureImageData *data = (CaptureImageData *) captureImageData;
    ICamera *camera = data->camera;
    
    // Allocate processing buffers
    IProcessedImage processedImage = sv_AllocateProcessedImage(sv_camera_GetImageInfo(camera));   

    framesCaptured = 0;
    
    while (streamActive) {
        // Get a frame using sv_camera_GetImage()
        image = sv_camera_GetImage(camera);
        
        // Check if frame is valid before using it
        if (image.data == NULL) {
            continue;
        }

        if (data->processing) {
            // Perform platform specific processing
            sv_ProcessImage(&image, &processedImage, SV_ALGORITHM_AUTODETECT);
        }

        printf(".");
        fflush(stdout);
        framesCaptured += 1;
        
        // Release a frame after it is no longer needed
        sv_camera_ReturnImage(camera, image);
    }

    // Deallocate processing buffer
    sv_DeallocateProcessedImage(&processedImage);

    return NULL;
}
