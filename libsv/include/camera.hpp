#pragma once
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>

#include "buffer.hpp"
#include "image_info_manager.hpp"
#include "sv/sv.h"

class Streamer;
class BufferAllocator;
class ImageFormat;
class FrameSize;
class FrameInterval;
class GenericControl;

class Camera : public ICamera
{
public:
    Camera(int fd, const char* name);
    Camera(int streamFd, int controlFd, const char* name, std::vector<int> pipeline = {});
    ~Camera();

    IImage GetImage() override;
    bool ReturnImage(IImage image) override;
    IImageInfo GetImageInfo() override;
    bool StartStream() override;
    bool StopStream() override;
    IControlList GetControlList() override;
    const char* GetName() override;
    const char* GetDriverName() override;
    IControl* GetControl(int id) override;

private:

    using BufferID = int;

    std::thread t;
    std::thread mt;
    std::condition_variable cv;
    std::condition_variable middleQcv;
    std::condition_variable filledQcv;
    std::mutex m;
    std::mutex qm;

    std::vector<Buffer> buffers;
    std::vector<Buffer> processedBuffers;
    std::queue<BufferID> middleBuffers;
    std::queue<BufferID> filledBuffers;

    std::unique_ptr<BufferAllocator> bufferAllocator;
    std::unique_ptr<Streamer> streamer;
    std::unique_ptr<ImageFormat> imageFormat;
    std::unique_ptr<FrameSize> frameSize;
    std::unique_ptr<FrameInterval> frameInterval;

    std::unique_ptr<ImageInfoManager> imageInfoManager;

    std::vector<std::unique_ptr<GenericControl>> controls;

    int streamFd;
    int controlFd;
    std::string name;
    std::string driverName;
    int blockingTimeOut = 0; // In milliseconds
    int tearingPreventionTimeout = 100; //In milliseconds

    bool isStreaming = false;
    bool isTerminated = false;
    bool isGetImageBlocking = true;
    bool isPreventTearingEnabled = true;

    int32_t GetBufferCount();
    bool SetBufferCount(int32_t count);

    bool ShouldAllocateOutputBuffers();
    bool AllocateBuffers();
    bool DeAllocateBuffers();

    void ReEnumerateFrameSize();
    void ReEnumerateFrameInterval();
    void ReEnumerateControls();

    void FrameThread();
    void MiddleThread();
    
    bool IsMultiplanar();
    void ReadDriverName();
    std::string ParseDriverName(v4l2_capability &capabilities);
    std::string ParseJetsonDriverName(std::string name);
    bool GetCapabilities(v4l2_capability &capabilities);
    void PopulateAPIControls();
    void PopulateMainV4L2Controls();
    void PopulateUserV4L2Controls();
    void PopulateExtControls();
    void PopulateControlDependencies();
    void SetControl(uint32_t id);
    void UpdateControlsAfterStartStream();

    GenericControl* GetControlByName(std::string name);

    void StopAllThreads();
    void StartAllThreads();
};
