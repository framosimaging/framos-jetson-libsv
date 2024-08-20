#ifndef INCLUDE_SV_SV_H
#define INCLUDE_SV_SV_H

#ifdef __cplusplus
#include <cstdint>
#include <vector>
#else
#include <stdint.h>
#endif
#include "enums.h"

#define MAJOR_VERSION 2
#define MINOR_VERSION 6
#define PATCH_VERSION 0
#define BUILD_VERSION 0

#define SV_EXPORT __attribute__ ((visibility ("default")))

/**
 * this needs to be kept in sync with kernel/nvidia/include/media/vi.h
 * from your current kernel
 */
#define EMBEDDED_DATA_MAX_SIZE ((8*64*1024))
#define EMBEDDED_DATA_BUFFER_ZONE_SIZE ((0x100))

/** @file sv.h
 *  Libsv is a streamlined V4L2 library. It provides simplified 
 *  and consistent access to V4L2 image sensor drivers on 
 *  multiple platforms.
 */

/**
 * @brief System uptime when the image was captured.
 */
struct Timestamp
{
    uint64_t s;                     /**< Seconds since uptime */
    uint64_t us;                    /**< Microseconds since last second */
};

/**
 * @brief This struct represents a single image
 * 
 * This structure is retrieved by calling GetImage() function
 */
struct IImage
{
    void* data;                     /**< Raw pointer to buffer */
    uint32_t id;                    /**< Image sequence number */
    uint32_t bufferid;              /**< Buffer id number */
    uint32_t length;                /**< Image length in bytes */
    uint32_t width;                 /**< Image width in pixels */
    uint32_t height;                /**< Image height in pixels */
    uint32_t pixelFormat;           /**< Image pixel format */
    uint32_t stride;                /**< Image stride, padding included */
    struct Timestamp timestamp;     /**< Image timestamp */
    void* embeddedData;             /**< Raw pointer to part of buffer with embeddedData*/
    uint32_t embeddedDataWidth;     /**< Embedded data width in pixels */
    uint32_t embeddedDataHeight;    /**< Embedded data height in pixels */
};

/**
 * @brief This struct represents an image that is a result of image processing
 * 
 * This structure is retrieved by calling AllocateProcessedImage() function
 */
struct IProcessedImage
{
    void* data;                     /**< Raw pointer to buffer */
    uint32_t length;                /**< Image length in bytes */
    uint32_t width;                 /**< Image width in pixels */
    uint32_t height;                /**< Image height in pixels */
    uint32_t pixelFormat;           /**< Image pixel format */
    uint32_t stride;                /**< Image stride, padding included */
    struct Timestamp timestamp;     /**< Image timestamp */
    void* embeddedData;             /**< Raw pointer to part of buffer with embeddedData*/
    uint32_t embeddedDataWidth;     /**< Embedded data width in bytes */
    uint32_t embeddedDataHeight;    /**< Embedded data height in bytes */
};

/**
 * @brief Contains information about image format
 * 
 * This structure is retrieved by calling GetImageInfo() function
 */
struct IImageInfo
{
    uint32_t length;                /**< Image length in bytes */
    uint32_t width;                 /**< Image width in pixels */
    uint32_t height;                /**< Image height in pixels */
    uint32_t pixelFormat;           /**< Image pixel format */
    uint32_t stride;                /**< Image stride, padding included */
};

/**
 * @brief This struct represents a single menu entry
 * 
 * This structure is retrieved by calling GetControlList() function
 */
struct MenuEntry
{
    const char* name;
    int32_t index;
};

// CPP

#ifdef __cplusplus

using MenuEntryList = std::vector<MenuEntry>;

/**
 * @brief This Class handles API defined and V4L2 defined controls
 * 
 * All available API controls can be found in the enums.h file while
 * V4L2 controls are camera/driver specific.
 */
class SV_EXPORT IControl
{
public:
    virtual ~IControl();
    virtual uint32_t GetID() = 0;
    virtual const char* GetName() = 0;
    virtual int64_t Get() = 0;
    virtual bool Set(int64_t val) = 0;
    virtual int64_t GetMinValue() = 0;
    virtual int64_t GetMaxValue() = 0;
    virtual int64_t GetStepValue() = 0;
    virtual int64_t GetDefaultValue() = 0;
    virtual MenuEntryList GetMenuEntries() = 0;
    virtual bool IsMenu() = 0;
};
using IControlList = std::vector<IControl*>;

/**
 * @brief Main class that represents a single camera
 */
class SV_EXPORT ICamera
{
public:
    virtual ~ICamera();
    virtual const char* GetName() = 0;          /**< see sv_camera_GetName()  */
    virtual const char* GetDriverName() = 0;    /**< see sv_camera_GetDriverName()  */
    virtual bool StartStream() = 0;             /**< see sv_camera_StartStream()  */
    virtual bool StopStream() = 0;              /**< see sv_camera_StopStream()  */
    virtual IImage GetImage() = 0;              /**< see sv_camera_GetImage()  */
    virtual bool ReturnImage(IImage image) = 0; /**< see sv_camera_ReturnImage()  */
    virtual IControlList GetControlList() = 0;  /**< see sv_camera_GetControlList()  */
    virtual IControl* GetControl(int id) = 0;   /**< see sv_camera_GetControl()  */
    virtual IImageInfo GetImageInfo() = 0;      /**< see sv_camera_GetImageInfo() */
};

using ICameraList = std::vector<ICamera*>;
#else
typedef struct IImage IImage;
typedef struct IProcessedImage IProcessedImage;
typedef struct IImageInfo IImageInfo;
typedef struct ICamera ICamera;
typedef struct IControl IControl;
typedef struct MenuEntry MenuEntry;
#endif

typedef IControl** CIControlList;
typedef MenuEntry* CMenuEntryList;
typedef ICamera** CICameraList;

#ifdef __cplusplus
extern "C" 
{
namespace sv
{
    extern SV_EXPORT ICameraList GetAllCameras();
    extern SV_EXPORT ICamera* GetCamera(const char* node);
    
    extern SV_EXPORT IProcessedImage AllocateProcessedImage(IImageInfo imageInfo);
    extern SV_EXPORT bool ProcessImage(const IImage& input, IProcessedImage& output, SV_PLATFORM_PROCESSING algorithm);
    extern SV_EXPORT void DeallocateProcessedImage(IProcessedImage& image);

    extern SV_EXPORT SV_PLATFORM GetPlatform();
}
#endif

// C

/**
 * @brief This function queries all available cameras.
 * 
 * This function looks for all nodes that start with the name
 * /dev/video* and creates ICamera* structures out from those 
 * names.
 * 
 * When using the C interface, this function returns a object
 * that was allocated with malloc. The user is responsible to 
 * free the object after using it (e.g. free(cameraList)).
 * 
 * @param size Pointer to integer in which the size of array will be stored.
 * @return CICameraList Returns a list of ICamera* objects or NULL. 
 */
extern SV_EXPORT CICameraList sv_GetAllCameras(int* size);
/**
 * @brief This function queries one single specific camera.
 * 
 * This function first calls internally sv_GetAllCameras() to
 * query all cameras and out of those cameras tries to find a 
 * name match.
 * 
 * @param node Name that is used as a matching factor (e.g. "/dev/video0").
 * @return ICamera* Returns a matched camera or NULL.
 */
extern SV_EXPORT ICamera* sv_GetCamera(const char* node);
/**
 * @brief This function queries the name of a camera.
 * 
 * Internally, all cameras are created out of node names found
 * in /dev/video*. To separated available cameras in a
 * multi-camera system, names are assigned and are equivalent
 * to their node names. 
 * 
 * @param camera 
 * @return const char* Returns a string containing camera name (e.g. "/dev/video0").
 */
extern SV_EXPORT const char* sv_camera_GetName(ICamera* camera);
/**
 * @brief This function queries the name of the driver that camera uses.
 * 
 * @param camera 
 * @return const char* Returns a string containing driver name.
 */
extern SV_EXPORT const char* sv_camera_GetDriverName(ICamera* camera);
/**
 * @brief This functions tells the camera to start capture frames.
 * 
 * This function first checks if all the necessary buffers are
 * allocated and if not, it tries to allocate them. After that
 * it calls the required V4L2 ioctls to start the capture process.
 * 
 * @param camera 
 * @return int Returns 1 if the function was successful otherwise 0.
 */
extern SV_EXPORT int sv_camera_StartStream(ICamera* camera);
/**
 * @brief This functions tells the camera to stop capture frames.
 * 
 * This function calls the requires V4L2 ioctls to stop the
 * ongoing capture process.
 * 
 * @param camera 
 * @return int Returns 1 if the function was successful otherwise 0.
 */
extern SV_EXPORT int sv_camera_StopStream(ICamera* camera);
/**
 * @brief This function queries a single frame from camera.
 * 
 * Image tearing may appear during stream on some settings due to V4L2 
 * not completely filling image buffers before returning them. LibSV 
 * ensures that the buffer is filled by waiting for the next image to be 
 * returned by the V4L2. The allowed wait time is limited by a timeout value.
 * This functionality can be toggled using the SV_API_PREVENT_TEARING control.
 * It is enabled by default. Timeout value can be adjusted using the 
 * SV_API_PREVENT_TEARING_TIMEOUT control.
 * 
 * This function is by default blocking. It waits indefinitely
 * until a new image is available in the finished queue. If a
 * timeout is required, setting the SV_API_BLOCKINGTIMEOUT
 * control to a value above 0 makes this function wait exactly
 * "n" ms for a new image.
 * 
 * This function can be non-blocking (i.e. it returns immediately
 * if no image is in the filled queue) by setting the value of
 * control SV_API_FETCHBLOCKING to 0.
 * 
 * A valid image doesn't have the "data" member set to null.
 * 
 * @param camera 
 * @return IImage Returns a valid or invalid image.
 */
extern SV_EXPORT IImage sv_camera_GetImage(ICamera* camera);
/**
 * @brief This function returns a queried frame.
 * 
 * Internally, this function uses the "bufferid" data member to
 * query the right buffer again to be used by the V4L2 driver.
 * It is strongly recommend that once this functions returns to
 * treat the image as invalid to avoid any possible fatal
 * problems.
 * 
 * @param camera 
 * @param image 
 * @return int Returns 1 if the function was successful otherwise 0
 */
extern SV_EXPORT int sv_camera_ReturnImage(ICamera* camera, IImage image);
/**
 * @brief This functions returns information about image format
 * 
 * This function provides information about current image format independently 
 * of image acquisition. It can be used to obtain this information before 
 * the acquisition starts or after it ends.
 * 
 * @param camera
 * @return IImageInfo
 */
extern SV_EXPORT IImageInfo sv_camera_GetImageInfo(ICamera* camera);
/**
 * @brief This function queries all available camera controls.
 * 
 * Camera controls are grouped into three distinct groups. 
 * 
 * The first control group starts with the ID 1, and they change
 * how the API behaves (e.g. GetImage() timeout). Their IDs are
 * available in the enums.sh file and use the "SV_API" namespace.
 * 
 * The second control group starts with the ID 10 and they change
 * the camera image format (e.g. Frame size). Their IDs are 
 * available in the enums.sh file and use the "SV_V4L2" namespace.
 * 
 * The third control group is dynamically created by the V4L2
 * and camera driver (e.g. Gain).
 * 
 * When using the C interface, this function returns a object
 * that was allocated with malloc. The user is responsible to 
 * free the object after using it (e.g. free(controlList)).
 * 
 * @param camera 
 * @param size Pointer to integer in which the size of array will be stored.
 * @return CIControlList Returns a list of IControl* objects or NULL.  
 */
extern SV_EXPORT CIControlList sv_camera_GetControlList(ICamera* camera, int* size);
/**
 * @brief This function queries a single camera control.
 * 
 * Internally, during the creation of the camera object, all
 * controls are queried and saved in a array. This function
 * matches the requested id with a existing control id.
 * 
 * @param camera 
 * @param id Id that is used as a matching factor
 * @return IControl* Returns a matched control or NULL.
 */
extern SV_EXPORT IControl* sv_camera_GetControl(ICamera* camera, int id);

extern SV_EXPORT uint32_t sv_control_GetID(IControl* control);
extern SV_EXPORT const char* sv_control_GetName(IControl* control);
extern SV_EXPORT int32_t sv_control_Get(IControl* control);
extern SV_EXPORT int sv_control_Set(IControl* control, int32_t val);
extern SV_EXPORT int32_t sv_control_GetMinValue(IControl* control);
extern SV_EXPORT int32_t sv_control_GetMaxValue(IControl* control);
extern SV_EXPORT int32_t sv_control_GetStepValue(IControl* control);
extern SV_EXPORT int32_t sv_control_GetDefaultValue(IControl* control);
/**
 * @brief This function quires all available menu entries
 * 
 * This function needs to be called again after changing Image format or
 * Frame size for the SV_V4L2_FRAMEINTERVAL control and after changing
 * Image format for the SV_V4L2_FRAMESIZE control.
 * 
 * When using the C interface, this function returns a object
 * that was allocated with malloc. The user is responsible to 
 * free the object after using it (e.g. free(menuEntries)).
 * 
 * @param control 
 * @param size Pointer to integer in which the size of array will be stored.
 * @return CMenuEntryList Returns a list of MenuEntry objects or NULL.
 */
extern SV_EXPORT CMenuEntryList sv_control_GetMenuEntries(IControl* control, int* size);
extern SV_EXPORT int sv_control_IsMenu(IControl* control);
/**
 * @brief This function allocates buffers for image processing based on provided image format information.
 * 
 * Processing buffers should be allocated after the image format is set. Otherwise there will be a mismatch 
 * between buffers acquired using GetImage() and allocated processing buffers. This will result in 
 * ProcessImage() function failing.
 * 
 * @param imageInfo
 * @return IProcessedImage
 */
extern SV_EXPORT IProcessedImage sv_AllocateProcessedImage(IImageInfo imageInfo);
/**
 * @brief This functions performs selected image processing
 * 
 * Pipelines on various platforms provide images in different formats 
 * that are often unknown to other software. This function converts such images
 * to a simple raw format that other software can use.
 * 
 * Processing does not alter the input image.
 * Deallocating an IProcessedImage does not affect the original IImage.
 * Returning an IImage does not affect the resulting IProcessedImage.
 * 
 * @param input
 * @param output
 * @param algorithm
 * @return bool True if processing successful, false otherwise.
 */
extern SV_EXPORT bool sv_ProcessImage(const IImage *input, IProcessedImage *output, enum SV_PLATFORM_PROCESSING algorithm);
/**
 * @brief This function deallocates a processing buffer allocated by AllocateProcessedImage()
 * 
 * This function has to be called when the processing buffer is no longer needed.
 * 
 * @param image
 */
extern SV_EXPORT void sv_DeallocateProcessedImage(IProcessedImage *image);
/**
 *  @brief This function returns the platform that the library detected on initialization
 */
extern SV_EXPORT enum SV_PLATFORM sv_GetPlatform();

#ifdef __cplusplus
}
#endif

#endif /*INCLUDE_SV_SV_H*/
