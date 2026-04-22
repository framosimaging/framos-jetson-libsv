#include <cstring>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

#include "easylogging++/easylogging++.h"

#include "camera.hpp"
#include "linux.hpp"
#include "platform.hpp"
#include "buffer_allocator.hpp"
#include "streamer.hpp"
#include "image_format.hpp"
#include "frame_size.hpp"
#include "frame_interval.hpp"
#include "v4l2_control_creator.hpp"
#include "generic_control.hpp"
#include "custom_control.hpp"
#include "string_util.hpp"

Camera::Camera(int fd, const char *name) : Camera(fd, fd, name) {}

Camera::Camera(int streamFd, int controlFd, const char* name, std::vector<int> pipeline)
: streamFd(streamFd), controlFd(controlFd), name(name)
{
    ReadDriverName();

    if (IsMultiplanar()) {
        bufferAllocator = std::unique_ptr<BufferAllocator>(new MplaneBufferAllocator(streamFd));
        streamer = std::unique_ptr<Streamer>(new MplaneStreamer(streamFd));
        imageInfoManager = std::unique_ptr<ImageInfoManager>(new MplaneSubdevImageInfoManager(streamFd, controlFd, pipeline));
        imageFormat = std::unique_ptr<ImageFormat>(new MplaneSubdevImageFormat(controlFd));
        frameSize = std::unique_ptr<FrameSize>(new MplaneSubdevFrameSize(controlFd));
        frameInterval = std::unique_ptr<FrameInterval>(new SubdevFrameInterval(controlFd));
    } else {
        bufferAllocator = std::unique_ptr<BufferAllocator>(new BufferAllocator(streamFd));
        streamer = std::unique_ptr<Streamer>(new Streamer(streamFd));
        imageInfoManager = std::unique_ptr<ImageInfoManager>(new ImageInfoManager(streamFd));
        imageFormat = std::unique_ptr<ImageFormat>(new ImageFormat(streamFd));
        frameSize = std::unique_ptr<FrameSize>(new FrameSize(streamFd));
        frameInterval = std::unique_ptr<FrameInterval>(new FrameInterval(streamFd));
    }

    PopulateMainV4L2Controls();
    PopulateUserV4L2Controls();
    PopulateAPIControls();

    PopulateControlDependencies();
}

Camera::~Camera()
{
    isTerminated = true;
    StopStream();
    linux::CloseFileDescriptor(streamFd);
    linux::CloseFileDescriptor(controlFd);
}

const char* Camera::GetName()
{
    return name.c_str();
}

const char* Camera::GetDriverName()
{
    return driverName.c_str();
}

const char* Camera::GetCardTypeInfo()
{
    return cardTypeInfo.c_str();
}

IControlList Camera::GetControlList()
{
    IControlList icontrolList;
    icontrolList.reserve(controls.size());
    for(auto& control : controls) {
        icontrolList.push_back(control.get());
    }
    return icontrolList;
}

void Camera::StartAllThreads()
{
    isStreaming = false;
    isTerminated = false;

    t = std::thread(&Camera::FrameThread, this);
    mt = std::thread(&Camera::MiddleThread, this);
}

void Camera::StopAllThreads()
{
    isStreaming = false;
    isTerminated = true;

    cv.notify_all();
    middleQcv.notify_all();
    filledQcv.notify_all();

    if (t.joinable()) {
        t.join();
    }

    if (mt.joinable()) {
        mt.join();
    }
}

bool Camera::AllocateBuffers()
{
    imageInfoManager->UpdateFromDevice();

    buffers = bufferAllocator->Allocate();
    if (buffers.empty()) {
        LOG(ERROR) << "buffer allocation failed";
        return false;
    }

    StartAllThreads();
    return true;
}

bool Camera::DeAllocateBuffers()
{
    StopAllThreads();

    bufferAllocator->Deallocate(buffers);
    buffers = {};

    return true;
}

bool Camera::StartStream()
{
    if(isStreaming) {
        LOG(WARNING) << "failed to start stream - already streaming";
        return false;
    }

    if (!DeAllocateBuffers()) {
        LOG(ERROR) << "failed to start stream - failed to deallocate buffers";
        return false;
    }

    if (!AllocateBuffers()) {
        LOG(ERROR) << "failed to start stream - failed to allocate buffers";
        return false;
    }

    while (!filledBuffers.empty()) {
        filledBuffers.pop();
    }

    while (!middleBuffers.empty()) {
        middleBuffers.pop();
    }

    for (auto& buff : buffers) {
        bool queuedStatus = true;
        auto res = streamer->AddToIncomingQueue(buff.index, &queuedStatus);
        if(!queuedStatus || !res) {
            LOG(ERROR) << "failed to start stream - failed to add to incoming queue";
            return false;
        }
    }

    if (!streamer->Start()) {
        LOG(ERROR) << "failed to start stream";
        return false;
    }
 
    switch (v4l2::platform) {
    case SV_PLATFORM_JETSON_TX2:
    case SV_PLATFORM_JETSON_XAVIER:
    case SV_PLATFORM_JETSON_AGX_ORIN:
    case SV_PLATFORM_JETSON_ORIN_NANO:
    case SV_PLATFORM_JETSON_ORIN_NX:
    case SV_PLATFORM_JETSON_NANO:
    case SV_PLATFORM_JETSON_XAVIER_NX:
    case SV_PLATFORM_JETSON_TX2NX:
        UpdateControlsAfterStartStream();
    default:
        break;
    } 

    isStreaming = true;
    cv.notify_all();
    return true;
}

bool Camera::StopStream()
{
    isStreaming = false;
    cv.notify_all();
    middleQcv.notify_all();
    filledQcv.notify_all();

    if (!streamer->Stop()) {
        return false;
    }

    if (!DeAllocateBuffers()) {
        return false;
    }

    return true;
}

IImage Camera::GetImage()
{
    IImage image = {};

    std::unique_lock<std::mutex> lck(qm);
    auto empty = filledBuffers.empty();

    if (empty && isGetImageBlocking) {
        //Block!
        if (blockingTimeOut == 0) {
            filledQcv.wait(lck, [&](){ return !filledBuffers.empty() || !isStreaming; });
        }
        else {
            filledQcv.wait_for(lck, std::chrono::milliseconds(blockingTimeOut), [&](){ return !filledBuffers.empty() || !isStreaming; });
        }

        if (!isStreaming) {
            return image; 
        }
    }
    else if (empty) {
        return image; 
    }

    auto id = filledBuffers.front();
    filledBuffers.pop();

    image.data = buffers[id].rawData;

    auto info = imageInfoManager->GetImageInfo();
    image.length = info.length;
    image.width = info.width;
    image.height = info.height;
    image.pixelFormat = info.pixelFormat;
    image.stride = info.stride;

    image.id = buffers[id].sequence;
    image.bufferid = buffers[id].index;
    image.timestamp = buffers[id].timestamp;

/*
XXX Beware, this is (hopefully) a temporary implementation of accessing embedded data on jetson tx2.
It relies on a custom patch in the VI4 driver on Jetson TX2
(commit 94ae61f218 in repo ssh://rhodecode@repo.smartek.vision/Projects/MP4ISB/Software/Jetson/kernel/nvidia.git)

The patch works by allocating some extra memory after each buffer, and the VI is modified to put the embedded data there.
The buffer zone is used to store the width and height of embedded data (2 bytes each),
but it has to be 0x100 bytes because that is the smallest increment by which the dma target addresses can be adjusted.
The embedded data is stored after the buffer "buffer zone".

buffer
+---------------------------------+
|pixel data                       |
|                                 |
+---------------------------------+
|buffer zone of 0x100 bytes       |
+---------------------------------+
|embedded data                    |
+---------------------------------+

 ASCII art generated using https://asciiflow.com/
 */

    uint16_t * bufferZonePtr = (uint16_t *)((uint8_t *)image.data + image.length);
    uint8_t * embeddedDataPtr = (uint8_t *)image.data + image.length + EMBEDDED_DATA_BUFFER_ZONE_SIZE;

    image.embeddedData = (void *)embeddedDataPtr;
    image.embeddedDataWidth = bufferZonePtr[0];
    image.embeddedDataHeight = bufferZonePtr[1];

#if 0
    printf("image.data = %llx, image.length = %d, embedded data start  = %llx, image.embeddedDataWidth = %d, image.embeddedDataHeight = %d",
                (long long unsigned int)image.data, image.length, (long long unsigned int)embeddedDataPtr, image.embeddedDataWidth, image.embeddedDataHeight);

    std::cout << "Camera::GetImage metadata dump start\n" << std::endl;

    for (size_t i = 0; i < image.embeddedDataHeight; ++i)
    {
        for (size_t j = 0; j < image.embeddedDataWidth; ++j)
        {
            std::cout << std::hex << (unsigned int)*((uint8_t *)image.embeddedData + i*image.embeddedDataWidth + j) << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nCamera::GetImage metadata dump end" << std::endl;
#endif 

    return image;
}

bool Camera::ReturnImage(IImage image)
{
    std::unique_lock<std::mutex> lck(qm);
    return streamer->AddToIncomingQueue(image.bufferid);
}

IImageInfo Camera::GetImageInfo()
{
    return imageInfoManager->GetImageInfo();
}

IControl* Camera::GetControl(int id)
{
    for (auto& control : controls) {
        if (control->GetID() == (uint32_t)id) {
            return dynamic_cast<IControl*>(control.get());
        }
    }
    return nullptr;
}

void Camera::FrameThread()
{
    while (!isTerminated)
    {
        {
            std::unique_lock<std::mutex> lck(m);
            cv.wait(lck, [&](){ return isStreaming || isTerminated; });
        }

        while (!isTerminated && isStreaming) {

            DqStatus dqStatus;
            isStreaming = streamer->GetFromOutgoingQueue(dqStatus);

            if (!isStreaming || isTerminated) {
                break;
            }

            if (dqStatus.error) {
                streamer->AddToIncomingQueue(dqStatus.bufferID);
                continue;
            }

            buffers[dqStatus.bufferID].sequence = dqStatus.sequence;
            buffers[dqStatus.bufferID].timestamp = dqStatus.timestamp;
                
            {
                std::unique_lock<std::mutex> lck(qm);
                middleBuffers.push(dqStatus.bufferID);
            }

            middleQcv.notify_all();
        }
    }
}

void Camera::MiddleThread()
{   
    while (!isTerminated)
    {
        {
            std::unique_lock<std::mutex> lck(m);
            cv.wait(lck, [&](){ return isStreaming || isTerminated; });
        }
        while (!isTerminated && isStreaming) {

            {
                std::unique_lock<std::mutex> lck(qm);
                middleQcv.wait(lck, [&](){ return !middleBuffers.empty() || isTerminated || !isStreaming; });
                auto predicate = middleQcv.wait_for(lck, std::chrono::milliseconds(tearingPreventionTimeout), [&]() { 
                    return !isPreventTearingEnabled || middleBuffers.size() >= 2 || !isStreaming || isTerminated; 
                });
                if (!predicate) {
                    LOG_N_TIMES(3, WARNING) << "potential tearing detected, consider increasing SV_API_PREVENT_TEARING_TIMEOUT";
                }
            }

            if (!isStreaming || isTerminated) {
                break;
            }

            auto bufferID = middleBuffers.front();
            
            {
                std::unique_lock<std::mutex> lck(qm);
                middleBuffers.pop();
                filledBuffers.push(bufferID);
            }

            filledQcv.notify_all();
        }
    }
}

void Camera::ReEnumerateFrameSize()
{
    auto frameSizeControl = std::find_if(
        controls.begin(), controls.end(), 
        [](std::unique_ptr<GenericControl> &control) { 
            return control->GetID() ==  SV_V4L2_FRAMESIZE; 
        }
    );

    if (frameSizeControl == controls.end()) {
        LOG(ERROR) << "failed to re enumerate frame size";
        return;
    }

    frameSizeControl[0]->Update(frameSize->EnumerateFrameSize(imageInfoManager->GetImageInfo().pixelFormat));

    ReEnumerateFrameInterval();
}

void Camera::ReEnumerateFrameInterval()
{
    auto frameIntervalControl = std::find_if(
        controls.begin(), controls.end(),
        [](std::unique_ptr<GenericControl> &control){
            return control->GetID() == SV_V4L2_FRAMEINTERVAL;
        }
    );

    if (frameIntervalControl == controls.end()) {
        LOG(ERROR) << "failed to re enumerate frame interval";
        return;
    }

    auto imageInfo = imageInfoManager->GetImageInfo();
    frameIntervalControl[0]->Update(frameInterval->EnumerateFrameInterval(imageInfo.pixelFormat, imageInfo.width, imageInfo.height));

    ReEnumerateControls();
}

void Camera::ReEnumerateControls()
{
    for (const auto& control : controls)
        control->Update();
}

bool Camera::IsMultiplanar()
{
    v4l2_capability capabilities = {};

    if (GetCapabilities(capabilities)) {
        return capabilities.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    } else {
        LOG(ERROR) << "Unable to determine if driver is multiplanar. Assuming single plane.";
        return false;
    }
}

void Camera::ReadDriverName()
{
    v4l2_capability capabilities = {};

    if (GetCapabilities(capabilities)) {
        driverName = ParseDriverName(capabilities);
    } else {
        LOG(ERROR) << "Unable to determine the name of the driver.";
        driverName = "unknown device";
    }
}

std::string Camera::ParseDriverName(v4l2_capability &capabilities)
{
    std::string name((char *)capabilities.card);
    switch(v4l2::platform) {
        case SV_PLATFORM_JETSON_TX2:
        case SV_PLATFORM_JETSON_XAVIER:
        case SV_PLATFORM_JETSON_NANO:
        case SV_PLATFORM_JETSON_XAVIER_NX:
        case SV_PLATFORM_JETSON_TX2NX:
        case SV_PLATFORM_JETSON_AGX_ORIN:
        case SV_PLATFORM_JETSON_ORIN_NANO:
        case SV_PLATFORM_JETSON_ORIN_NX:
            GetCardTypeInfo(name);
            return ParseJetsonDriverName(name);
        default:
            return name;
    }
}

std::string Camera::ParseJetsonDriverName(std::string name)
{
    auto split = SplitString(name, " ");
    if (split.size() == 3) {
        return split[1];
    }

    LOG(WARNING) << "Unable to parse Jetson driver name. Using unparsed name.";
    return name;
}

bool Camera::GetCapabilities(v4l2_capability &capabilities)
{
    capabilities = {};

    if (::ioctl(streamFd, VIDIOC_QUERYCAP, &capabilities)) {
        LOG(ERROR) << "VIDIOC_QUERYCAP failed";
        return false;
    }

    return true;
}

void Camera::GetCardTypeInfo(std::string info)
{
    auto split = SplitString(info, " ");
    if (split.size() == 3) {
        cardTypeInfo = split[2];
        return;
    }

    LOG(WARNING) << "Unable to parse Jetson device info. Using unparsed info.";
    cardTypeInfo =  info;
}

void Camera::PopulateMainV4L2Controls()
{
    auto imageInfo = imageInfoManager->GetImageInfo();

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_V4L2_IMAGEFORMAT,
        "Image Format",
        imageFormat->EnumerateImageFormat(),
        [this]() 
        { 
            return imageFormat->GetImageFormatIndex(imageInfoManager->GetImageInfo().pixelFormat); 
        },
        [this](int32_t val)
        { 
            if (isStreaming) {
                LOG(ERROR) << "failed to set image format - streaming in progress";
                return false;
            }

            if (!DeAllocateBuffers()) {
                LOG(ERROR) << "failed to set image format - unable to deallocate buffers";
                return false;
            }

            if (imageInfoManager->SetPixelFormat(imageFormat->GetImageFormat(val)) == false) {
                LOG(ERROR) << "failed to set image format";
                return false;
            }

            ReEnumerateFrameSize();
            return true;
        }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_V4L2_FRAMESIZE,
        "Frame Size",
        frameSize->EnumerateFrameSize(imageInfo.pixelFormat),
        [this]() 
        {
            auto imageInfo = imageInfoManager->GetImageInfo();
            return frameSize->GetFrameSizeIndex(imageInfo.pixelFormat, imageInfo.width, imageInfo.height);
        },
        [this](int32_t val)
        { 
            if (isStreaming) {
                LOG(ERROR) << "failed to set frame size - streaming in progress";
                return false;
            }

            if (!DeAllocateBuffers()) {
                LOG(ERROR) << "failed to set image format - unable to deallocate buffers";
                return false;
            }

            auto imageInfo = imageInfoManager->GetImageInfo();
            if (imageInfoManager->SetFrameSize(frameSize->GetWidth(imageInfo.pixelFormat, val), frameSize->GetHeight(imageInfo.pixelFormat, val)) == false) {
                LOG(ERROR) << "failed to set image format";
                return false;
            }

            ReEnumerateFrameInterval();
            return true;
        }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_V4L2_FRAMEINTERVAL,
        "Frame Interval",
        frameInterval->EnumerateFrameInterval(imageInfo.pixelFormat, imageInfo.width, imageInfo.height),
        [this]() { return 0; },
        [this](int32_t val) { (void)val; return true; }
    )));

    SetControl(SV_V4L2_IMAGEFORMAT);
    SetControl(SV_V4L2_FRAMESIZE);
}

void Camera::PopulateAPIControls()
{
    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_API_BUFFERCOUNT,
        "Buffer Count",
        3,
        1000,
        1,
        bufferAllocator->GetBufferCount(),
        [this]() { return this->GetBufferCount(); },
        [this](int32_t val) { return this->SetBufferCount(val); }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_API_FETCHBLOCKING,
        "Fetch Blocking",
        0,
        1,
        1,
        0,
        [this]() { return this->isGetImageBlocking; },
        [this](int32_t val) { this->isGetImageBlocking = val; return true; }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_API_BLOCKINGTIMEOUT,
        "Blocking Timeout",
        0,
        INT32_MAX,
        1,
        blockingTimeOut,
        [this]() { return this->isGetImageBlocking; },
        [this](int32_t val) { this->isGetImageBlocking = val; return true; }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_API_PREVENT_TEARING,
        "Prevent Tearing",
        0,
        1,
        1,
        1,
        [this]() { return this->isPreventTearingEnabled; },
        [this](int32_t val) { this->isPreventTearingEnabled = val; return true; }
    )));

    controls.push_back(std::unique_ptr<CustomControl>(new CustomControl(
        SV_API_PREVENT_TEARING_TIMEOUT,
        "Prevent Tearing Timeout",
        0,
        INT32_MAX,
        1,
        tearingPreventionTimeout,
        [this]() { return this->tearingPreventionTimeout; },
        [this](int32_t val) { this->tearingPreventionTimeout = val; return true; }
    )));
}

void Camera::PopulateUserV4L2Controls()
{
    auto userControls = V4l2ControlCreator().CreateControls(controlFd);
    controls.insert(controls.end(), std::make_move_iterator(userControls.begin()), std::make_move_iterator(userControls.end()));
}

void Camera::PopulateControlDependencies()
{
    auto fpsControl = GetControlByName("Frame Rate");
    if (fpsControl) {
        auto exposureControl = GetControlByName("Exposure");
        if (exposureControl) {
            fpsControl->AddDependency(exposureControl);
        } 
    }
}

void Camera::SetControl(uint32_t id)
{
    auto controlIterator = std::find_if(controls.begin(), controls.end(), [&id] (std::unique_ptr<GenericControl> &control){ return control->GetID() == id; });
    if (controlIterator != controls.end()) { 
        auto &control = *controlIterator;
        control->Set(control->Get());
    }
}

void Camera::UpdateControlsAfterStartStream()
{
    for (const auto& name : { "Frame Rate", "Exposure" }) {
        auto control = GetControlByName(name);
        if (control)
            control->Update();
    }
}

GenericControl* Camera::GetControlByName(std::string name)
{
    auto controlIterator = std::find_if(controls.begin(), controls.end(), [&name] (std::unique_ptr<GenericControl> &control){ return control->GetName() == name; });
    if (controlIterator != controls.end())
        return (*controlIterator).get();
    else 
        return nullptr;
}

int32_t Camera::GetBufferCount()
{
    return bufferAllocator->GetBufferCount();
}

bool Camera::SetBufferCount(int32_t count)
{
    if (isStreaming) {
        LOG(ERROR) << "failed to set buffer count - streaming in progress";
        return false;
    }

    bufferAllocator->SetBufferCount(count);
    
    DeAllocateBuffers();

    return true;
}
