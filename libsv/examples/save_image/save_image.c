#include "common_c/common.h"
#include "sv/sv.h"
#include <sys/stat.h>
#include <fcntl.h>

void GetCurrentWorkingDir(char *dir);
void SaveFrame(void *data, uint32_t length, char *name, char *folder);

int main() 
{
    const int minFrameNumber = 1;
    const int maxFrameNumber = 1000;
    const int defFrameNumber = 10;
    int numCameras;
    int numControl;
    bool processing;
    bool saveEmbeddedData;
    void *data;
    uint32_t length;
    void *embeddedData = NULL;
    uint32_t lengthEmbeddedData = 0;
    int i;
    
    // Use sv_GetAllCameras(int* size) to get a list of available cameras
    CICameraList cameras = sv_GetAllCameras(&numCameras);
    if (numCameras == 0) {
        printf("No cameras detected! Exiting...\n");
        return 0;
    }
    
    // Select one of available cameras 
    ICamera *camera = SelectCamera(&cameras, numCameras);
    
    processing = SelectEnable("platform-specific processing", false);

    saveEmbeddedData = SelectEnable("save embedded data", false);

    // Select one of the available image formats
    IControl *control = sv_camera_GetControl(camera, SV_V4L2_IMAGEFORMAT);
    if (control) {
        SelectPixelFormat(control);
    }
    
    // Use sv_camera_GetControlList() to get a list of available controls
    CIControlList controls = sv_camera_GetControlList(camera, &numControl);

    // Use SV_V4L2_FRAMESIZE to set resolution
    SelectFrameSize(sv_camera_GetControl(camera, SV_V4L2_FRAMESIZE));

    // Set other controls matched by name
    const char *queryControls[] = {"Gain", "Digital Gain", "Exposure", "Coarse Time", "Frame Length", "Black Level"};
    
    for (i = 0; i < numControl; i++) {
        const char *name = sv_control_GetName((IControl*)controls[i]); 
        if (CheckQueryBool(name, strlen(name), queryControls, ARRAY_LENGTH(queryControls))) {
            int32_t value = SelectValue(name, sv_control_GetMinValue((IControl*)controls[i]), sv_control_GetMaxValue((IControl*)controls[i]), sv_control_GetDefaultValue((IControl*)controls[i]));
            sv_control_Set((IControl*)controls[i], value);
        }
    }
    
    // Use sv_camera_StartStream() to start acquiring frames
    if (sv_camera_StartStream(camera) == 0) {
        printf("Failed to start stream!\n");
        return 0;
    }

    const int frameCount = SelectValue("Numbers of frames you wish to save", minFrameNumber, maxFrameNumber, defFrameNumber);
    char folder[PATH_MAX];
    GetCurrentWorkingDir(folder);
    strcat(folder, "/output/");
    
    // Allocate processing buffers
    IProcessedImage processedImage = sv_AllocateProcessedImage(sv_camera_GetImageInfo(camera));

    int frameSaved = 0;
    char frameNaming[256];
    for (i = 0; i < frameCount; i++) {
        
        IImage image = sv_camera_GetImage(camera);

        if (image.data == NULL) {
            printf("Unable to save frame, invalid image data\n");
            continue;
        }

        if (processing) {
            sv_ProcessImage(&image, &processedImage, SV_ALGORITHM_AUTODETECT);
            data = processedImage.data;
            length = processedImage.length;
        } else {
            data = image.data;
            length = image.length;
        }

        if (saveEmbeddedData) {
            if (processing) {
                embeddedData = processedImage.embeddedData;
                lengthEmbeddedData = processedImage.embeddedDataWidth * processedImage.embeddedDataHeight;
            } else {
                embeddedData = image.embeddedData;
                lengthEmbeddedData = image.embeddedDataWidth * image.embeddedDataHeight * 2;
            }
        }

        sprintf(frameNaming, "frame%d.raw", i);
        SaveFrame(data, length, frameNaming, folder);

        if (saveEmbeddedData) {
            if (lengthEmbeddedData != 0) {
                sprintf(frameNaming, "embedded%d.raw", i);
                SaveFrame(embeddedData, lengthEmbeddedData, frameNaming, folder);
            } else {
                printf("embedded data length is zero\n");
            }
        }

        printf(".");
        fflush(stdout);
        frameSaved += 1;

        sv_camera_ReturnImage(camera, image);
    }

    sv_camera_StopStream(camera);

    sv_DeallocateProcessedImage(&processedImage);

    printf("\nSaved %d frames to %s folder.\n", frameSaved, folder);

    return 0;
}

void GetCurrentWorkingDir(char *dir)
{
    char def[] = "";

    if (getcwd(dir, PATH_MAX) == NULL) {
        strcpy(dir, def);
    }
}

void SaveFrame(void *data, uint32_t length, char *name, char *folder)
{
    mkdir(folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    
    char file[PATH_MAX];
    strcpy(file, folder);
    strcat(file, name);
    
    remove(file);
    int fd = open (file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        printf("Unable to save frame, cannot open file %s\n", file);
        return;
    }
        
    int writenBytes = write(fd, data, length); 
    if (writenBytes < 0) {
        printf("Error writing to file %s\n", file);
    } else if ( (uint32_t) writenBytes != length) {
        printf("Warning: %d out of %d were written to file %s\n", writenBytes, length, file);
    }

    close(fd);
}
