#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <linux/videodev2.h>
#include "nvbufsurface.h"

#define NV12_PLANES 2
#define MAX_CAPTURE_BUFFFERS 10

namespace v4l2isp
{
    struct FrameSize
    {
        uint32_t width;
        uint32_t height;
    };

	struct SensorMode{
		uint32_t index;
		uint32_t frameRate;
		FrameSize frameSize;		
	};


	struct Control{
		std::string name;
		uint32_t id;
		uint32_t min;
		uint32_t max;
	};

		
	struct Image
	{
		uint32_t index;
		uint32_t strideY;
		uint32_t strideUV;
		char *planeY;
		char *planeUV;
	};
			

    class Camera
    {
        public:
            Camera(std::string node);
            ~Camera();
            Camera(const Camera&) = delete;
            Camera& operator=(const Camera&) = delete;
            std::string GetName();
            std::string GetDriverName();
            std::vector<uint32_t> GetPixelformats();
            void SetPixelformat(uint32_t pixelformat);
            void SetFrameSize(const FrameSize& frameSize);
            void StartStream();
            void StopStream();
            Image *GetImage(); 
            void ReturnImage();
            uint32_t GetWidth();
            uint32_t GetHeight();
            std::vector<Control> GetControls();
            void SetControl(Control control);
            void SetFrameRate(uint32_t framerate);
            FrameSize SelectFrameSize();
              
		private:
            void SetFormat();
            void GetSensorModes(int pipeID);
            void SetSensorMode(uint32_t sensorMode);
            void FreeBuffers();
            int32_t fd = -1;
            Image image {};     
            std::string name;
            std::string driverName;
            v4l2_buf_type bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            uint32_t memoryType = V4L2_MEMORY_DMABUF;
            uint32_t width = 1920;
            uint32_t height = 1080;
            uint32_t pixelformat = V4L2_PIX_FMT_NV12M;
            bool formatSet = false;
            std::vector <Control> controls;
            int32_t dmabuffers_fd[MAX_CAPTURE_BUFFFERS] = {};
            uint32_t numOfBuffers = 0;
            NvBufSurface *nvbuf_surf = nullptr;
			std::vector<SensorMode> sensorModes;
            std::atomic<bool> streaming {false};
			uint32_t sensorMode = 0;
         
    };

	std::vector<std::string> GetAllCameras();

    class StdOut
    {
        public:
            StdOut() : saved(false) {}
            ~StdOut() = default;
            bool Disable(int &pipeID);
            bool Restore();
        private:
            int savedStdout;
            bool saved;
    };
    
    
}
