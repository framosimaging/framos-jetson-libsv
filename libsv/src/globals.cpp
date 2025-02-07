#include "globals.hpp"
#include "linux.hpp"
#include "camera.hpp"
#include "platform.hpp"
#include "mediapipeline.hpp"

#include "easylogging++/easylogging++.h"

#include <fstream>
#include <regex>

INITIALIZE_EASYLOGGINGPP

namespace global
{
    void Global::DetectPlatform()
    {
        std::ifstream modelFile("/sys/firmware/devicetree/base/model");
        if (!modelFile.good()) {
            LOG(ERROR) << "failed to open file for platform detection";
            return;
        }

        std::string model;
        std::getline(modelFile, model);
        if  (std::regex_match(model, std::regex("^quil.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_TX2;
        } else if (std::regex_match(model, std::regex("^Qualcomm Technologies, Inc. APQ 8016 SBC.*"))) {
            v4l2::platform = SV_PLATFORM_DRAGONBOARD_410C;
        } else if (std::regex_match(model, std::regex("^Jetson-AGX.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_XAVIER;
        } else if (std::regex_match(model, std::regex("^NVIDIA Jetson Nano Developer Kit.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_NANO;
        } else if (std::regex_match(model, std::regex("^NVIDIA Jetson Xavier NX Developer Kit.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_XAVIER_NX;
        } else if (std::regex_match(model, std::regex("^lanai.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_TX2NX;
        } else if (std::regex_match(model, std::regex("^NVIDIA Jetson AGX Orin.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_AGX_ORIN;
        } else if (std::regex_match(model, std::regex("^NVIDIA Jetson Orin Nano.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_ORIN_NANO;
        } else if (std::regex_match(model, std::regex("^NVIDIA Jetson Orin NX.*"))) {
            v4l2::platform = SV_PLATFORM_JETSON_ORIN_NX;
        } else {
            LOG(ERROR) << "failed to match a supported platform";
        }

        modelFile.close();
    }

    void Global::InitializeLogging()
    {
        el::Configurations configuration;
        configuration.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
        configuration.setGlobally(el::ConfigurationType::ToFile, "true");
        configuration.setGlobally(el::ConfigurationType::Format, "[%datetime] %level: %msg (%fbase:%line)");
        configuration.setGlobally(el::ConfigurationType::Filename, "/tmp/libsv/log");
        configuration.setGlobally(el::ConfigurationType::MaxLogFileSize, "2097152");
        configuration.setGlobally(el::ConfigurationType::LogFlushThreshold, "1");
        el::Loggers::reconfigureAllLoggers(configuration);
    }

    void Global::InitializeCameras()
    {
        DetectPlatform();

        switch (v4l2::platform) {
            case SV_PLATFORM_JETSON_TX2:
            case SV_PLATFORM_JETSON_XAVIER:
            case SV_PLATFORM_JETSON_NANO:
            case SV_PLATFORM_JETSON_XAVIER_NX:
            case SV_PLATFORM_JETSON_TX2NX:
            case SV_PLATFORM_JETSON_AGX_ORIN:
            case SV_PLATFORM_JETSON_ORIN_NANO:
            case SV_PLATFORM_JETSON_ORIN_NX:
                InitializeJetsonCameras();
                break;
            case SV_PLATFORM_DRAGONBOARD_410C:
                InitializeDragonBoardCameras();
                break;
            case SV_PLATFORM_UNKNOWN:
                LOG(ERROR) << "failed to detect platform";
                break;
        }
    }

    void Global::InitializeJetsonCameras()
    {
        auto files = linux::GetFilesInDir("/dev", "video*");
        for(auto& file : files) {
            cameras.push_back(std::unique_ptr<Camera>( new Camera(
                linux::OpenFileDescriptor(file.c_str()),
                file.c_str()
            )));
        }
    }

    void Global::InitializeDragonBoardCameras()
    {
        media::MediaPipeline mediaPipeline("/dev/media0");

        auto sensorEntities = mediaPipeline.FilterEntities(
            [](const media::MediaEntity& e){
                return e.GetType() == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
            }
        );
        for (const auto& sensorEntity : sensorEntities) {
            uint32_t index = GetSensorIndex(sensorEntity, mediaPipeline);
            auto streamNode = GetStreamNode(index, mediaPipeline);
            auto controlNode = linux::GetDevnodeName(sensorEntity.GetMajor(), sensorEntity.GetMinor());
            InitializeDragonBoardCamera(index, streamNode, controlNode, mediaPipeline);
        }
    }

    uint32_t Global::GetSensorIndex(const media::MediaEntity& sensorEntity, const media::MediaPipeline& mediaPipeline)
    {
        auto links = mediaPipeline.GetOutboundLinkedEntities(sensorEntity);
        for (auto link : links) {
            std::string name = link.GetName();
            std::smatch match;
            std::string regexPattern = "msm_csiphy([0-9]+)";
            if (std::regex_search(name, match, std::regex(regexPattern)))
                try {
                    return std::stoi(match[1]);
                }
                catch (...) {

                }
        }

        LOG(WARNING) << "Unable to acquire index from links of entity " << sensorEntity.GetName() << ", assuming index 0";
        return 0;
    }

    std::string Global::GetStreamNode(uint32_t index, const media::MediaPipeline& mediaPipeline)
    {
        std::string vfeNode = "msm_vfe0_video" + std::to_string(index);
        auto filtered = mediaPipeline.FilterEntitiesByName(vfeNode);

        if (filtered.empty()) {
            std::string node = "/dev/video/" + std::to_string(index);
            LOG(WARNING) << "Unable to determine video node, assuming " << node;
            return node;
        }

        std::string node = linux::GetDevnodeName(filtered[0].GetMajor(), filtered[0].GetMinor());
        if (filtered.size() > 1)
            LOG(WARNING) << "More than one potential video node found, selecting" << node;
        return node;
    }

    void Global::InitializeDragonBoardCamera(uint32_t index, std::string streamNode, std::string controlNode, media::MediaPipeline &mediaPipeline)
    {
        std::string phyNode = "/dev/v4l-subdev" + std::to_string(index);
        std::string csiNode = "/dev/v4l-subdev" + std::to_string(index + 2);
        std::string ispifNode = "/dev/v4l-subdev" + std::to_string(index + 4);
        std::string vfeNode = "/dev/v4l-subdev" + std::to_string(index + 6);

        if (!linux::CheckFileDescriptor(streamNode.c_str())) return;
        if (!linux::CheckFileDescriptor(controlNode.c_str())) return;
        if (!linux::CheckFileDescriptor(phyNode.c_str())) return;
        if (!linux::CheckFileDescriptor(csiNode.c_str())) return;
        if (!linux::CheckFileDescriptor(ispifNode.c_str())) return;
        if (!linux::CheckFileDescriptor(vfeNode.c_str())) return;

        std::vector<int> pipeline;
        pipeline.push_back(linux::OpenFileDescriptor(phyNode.c_str()));
        pipeline.push_back(linux::OpenFileDescriptor(csiNode.c_str()));
        pipeline.push_back(linux::OpenFileDescriptor(ispifNode.c_str()));
        pipeline.push_back(linux::OpenFileDescriptor(vfeNode.c_str()));

        mediaPipeline.EnableLink("msm_csiphy" + std::to_string(index), "msm_csid" + std::to_string(index));
        mediaPipeline.EnableLink("msm_csid" + std::to_string(index), "msm_ispif" + std::to_string(index));
        mediaPipeline.EnableLink("msm_ispif" + std::to_string(index), "msm_vfe0_rdi" + std::to_string(index));

        cameras.push_back(std::unique_ptr<Camera>( new Camera (
            linux::OpenFileDescriptor(streamNode.c_str()),
            linux::OpenFileDescriptor(controlNode.c_str()),
            streamNode.c_str(), pipeline
        )));
    }

    std::vector<Camera*> Global::GetCameras()
    {
        std::vector<Camera*> cameraList;

        if (initialized == false) {
            InitializeLogging();
            InitializeCameras();
        }

        cameraList.reserve(cameras.size());
        for(auto& camera : cameras) {
            cameraList.push_back(camera.get());
        }
        return cameraList;
    }

    std::vector<std::unique_ptr<Camera>> Global::cameras;
    bool Global::initialized = false;
}
