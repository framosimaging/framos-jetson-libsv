#pragma once
#include <vector>
#include <memory>

namespace media
{
    class MediaEntity;
    class MediaPipeline;
}

class Camera;
namespace global
{
    class Global
    {
    public:
        static std::vector<Camera*> GetCameras();
 
    private:
        static void DetectPlatform();
        static void InitializeLogging();
        static void InitializeCameras();
        static void InitializeJetsonCameras();
        static void InitializeDragonBoardCameras();
        static uint32_t GetSensorIndex(const media::MediaEntity& sensorEntity, const media::MediaPipeline& mediaPipeline);
        static std::string GetStreamNode(uint32_t index, const media::MediaPipeline& mediaPipeline);
        static void InitializeDragonBoardCamera(uint32_t index, std::string streamNode, std::string controlNode, media::MediaPipeline &mediaPipeline);

        static bool initialized;
        static std::vector<std::unique_ptr<Camera>> cameras;
    };
}
