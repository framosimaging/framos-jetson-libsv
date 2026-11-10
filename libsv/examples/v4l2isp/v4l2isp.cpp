#include "v4l2isp/v4l2isp.hpp"
#include "v4l2isp/common.hpp"
#include "libv4l2.h"
#include "v4l2_nv_extensions.h"
#include <algorithm>
#include <cmath>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <fnmatch.h>
#include <linux/videodev2.h>
#include <sys/mman.h>


namespace v4l2isp
{

std::vector<std::string> GetAllCameras()
{
	std::vector<std::string> cameras;
	auto directory = ::opendir("/dev");
    if (directory) {
        auto file = ::readdir(directory);
        while (file) {
            if (::fnmatch("video*", file->d_name, 0) == 0) {
                cameras.push_back("/dev/" + std::string(file->d_name));
            }
            file = ::readdir(directory);
        }
        ::closedir(directory);
    }
    std::sort(cameras.begin(), cameras.end(), 
    [] (std::string first, std::string second) {return first < second;} );
    return cameras;
}


Camera::Camera(std::string node)
 : name(node)
{
    /**
     * v4l2_open prints available modes when executed
     */
    StdOut stdOut;
    int pipeId = -1;
    stdOut.Disable(pipeId);
    fd = ::v4l2_open(node.c_str(), O_RDWR);


    if (fd == -1) {
        stdOut.Restore();
        if (pipeId != -1)
            close(pipeId);
        throw std::runtime_error("Failed to open " + node);
    }

    try {
        GetSensorModes(pipeId);

        v4l2_capability capabilities {};
        if(::v4l2_ioctl(fd , VIDIOC_QUERYCAP, &capabilities) == -1) {
            throw std::runtime_error("VIDIOC_QUERYCAP failed");
        }
        if(!(capabilities.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
            throw std::runtime_error("Device does not support V4L2_CAP_VIDEO_CAPTURE_MPLANE");
        }
        driverName = reinterpret_cast<char*>(capabilities.card);
    } catch (...) {
        stdOut.Restore();
        if (pipeId != -1) {
            close(pipeId);
        }
        v4l2_close(fd);
        throw;
    }

    stdOut.Restore();
    if (pipeId != -1) {
        close(pipeId);
    }
}

Camera::~Camera()
{
    if (streaming) {
        try {
            StopStream();
        } catch (...) {

        }
    }

    FreeBuffers();
    if (fd != -1) {
	    ::v4l2_close(fd);
    }
}

std::string Camera::GetName()
{
    return name;
}

std::string Camera::GetDriverName()
{
    return driverName;
}

std::vector<uint32_t> Camera::GetPixelformats()
{
    std::vector<uint32_t> pixelformats;
    v4l2_fmtdesc formatDescription = {};
    formatDescription.index = 0;
    formatDescription.type = bufferType;
    while(::v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &formatDescription) == 0) {
        pixelformats.push_back(formatDescription.pixelformat);
        ++formatDescription.index;
    }
    return pixelformats;
}

void Camera::SetPixelformat(uint32_t pixelformat)
{
    this->pixelformat = pixelformat;
}

void Camera::SetFrameSize(const FrameSize& frameSize)
{
    width = frameSize.width;
    height = frameSize.height;
    SetFormat();
}

void Camera::SetFormat()
{
    struct v4l2_format format {};
    format.type = bufferType;
    format.fmt.pix_mp.width = width;
    format.fmt.pix_mp.height = height;
    format.fmt.pix_mp.pixelformat = pixelformat;
    format.fmt.pix_mp.num_planes = NV12_PLANES;
    if (::v4l2_ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        throw std::runtime_error("Failed to set format");
    }
    formatSet = true;
}

void Camera::StartStream()
{
    if (streaming) {
        return;
    }

	if (!formatSet) {
        SetFormat();
    }
	
	struct v4l2_requestbuffers reqbuffers {};
	reqbuffers.count = MAX_CAPTURE_BUFFFERS;
    reqbuffers.memory = memoryType;
    reqbuffers.type = bufferType;
    
    if (::v4l2_ioctl (fd, VIDIOC_REQBUFS, &reqbuffers) == -1) {
        throw std::runtime_error("VIDIOC_REQBUFS failed");
    }

    if (reqbuffers.count == 0) {
        throw std::runtime_error("Driver allocated no buffers");
    }

	numOfBuffers = std::min<uint32_t>(reqbuffers.count, MAX_CAPTURE_BUFFFERS);

    NvBufSurfaceAllocateParams cParams = {};

    try {
        for (uint32_t i = 0; i < numOfBuffers; ++i) {
            cParams.params.width = width;
            cParams.params.height = height;
            cParams.params.layout = NVBUF_LAYOUT_PITCH;
            cParams.params.colorFormat = NVBUF_COLOR_FORMAT_NV12;
            cParams.params.memType = NVBUF_MEM_SURFACE_ARRAY;
            cParams.memtag = NvBufSurfaceTag_CAMERA;
            if (NvBufSurfaceAllocate(&nvbuf_surf, 1, &cParams) != 0) {
                throw std::runtime_error("Failed to Create NvBufSurface");
            }
            nvbuf_surf->numFilled = 1;
            dmabuffers_fd[i] = nvbuf_surf->surfaceList[0].bufferDesc;
        }

        struct v4l2_plane captureplanes[NV12_PLANES];
        for (uint32_t i = 0; i < numOfBuffers; ++i) {
            v4l2_buffer buffer = {};
            buffer.index = i;
            buffer.type = bufferType;
            buffer.memory = memoryType;
            buffer.m.planes = captureplanes;
            buffer.length = 2;
            buffer.m.planes[0].m.fd = dmabuffers_fd[i];

            if(::v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buffer) == -1) {
                throw std::runtime_error("Failed to query buffers");
            }
        }
        
        if(::v4l2_ioctl (fd, VIDIOC_STREAMON, &bufferType) == -1) {
            throw std::runtime_error("Failed to start stream");
        }
        
        for (uint32_t i = 0; i < numOfBuffers; ++i) {
            struct v4l2_buffer queue_cap_v4l2_buf = {};
            struct v4l2_plane queue_cap_planes[2] = {};

            queue_cap_v4l2_buf.index = i;
            queue_cap_v4l2_buf.m.planes = queue_cap_planes;
            queue_cap_v4l2_buf.m.planes[0].m.fd = dmabuffers_fd[i];
            
            queue_cap_v4l2_buf.type = bufferType;
            queue_cap_v4l2_buf.memory = memoryType;
            queue_cap_v4l2_buf.length = 2;

            if(::v4l2_ioctl (fd, VIDIOC_QBUF, &queue_cap_v4l2_buf) == -1) {
                throw std::runtime_error("Failed to enqueue buffer");
            }
        } 		
        streaming = true;
    } catch (...) {
        FreeBuffers();
        throw;
    }
}

void Camera::StopStream()
{
    if(!streaming) {
        return;
    }
    streaming = false;
    
    try {
        if (::v4l2_ioctl(fd, VIDIOC_STREAMOFF, &bufferType) == -1) {
            throw std::runtime_error("Failed to stop stream");
        }

        struct v4l2_requestbuffers requestBuffers {};
        requestBuffers.type = bufferType;
        requestBuffers.memory = memoryType;
        requestBuffers.count = 0;
        if (::v4l2_ioctl(fd, VIDIOC_REQBUFS, &requestBuffers) == -1) {
            throw std::runtime_error("Failed to request 0 buffers");
        }
    } catch (...) {
        FreeBuffers();
        throw;
    }
    FreeBuffers();
}

Image* Camera::GetImage()
{  
    v4l2_buffer v4l2_buf = {};
    v4l2_plane planes[NV12_PLANES] = {};
	
    v4l2_buf.m.planes = planes;
    v4l2_buf.length = NV12_PLANES;
	v4l2_buf.type = bufferType;
	v4l2_buf.memory = memoryType;


	if (::v4l2_ioctl (fd, VIDIOC_DQBUF, &v4l2_buf) == -1) {
			throw std::runtime_error("Failed to dequeue buffer");
	}
    
    image.index = v4l2_buf.index;
    if (image.index >= numOfBuffers) {
        throw std::runtime_error("Driver returned out-of-range buffer index");
    }
    image.planeY = nullptr;
    image.planeUV = nullptr;

    int dmabuf_fd = v4l2_buf.m.planes[0].m.fd;
    if (dmabuf_fd <= 0) {
        try {
            ReturnImage();
        } catch (...) {

        }
        throw std::runtime_error("dmabuf_fd <= 0");
    }

    if (NvBufSurfaceFromFd(dmabuf_fd, (void**)(&nvbuf_surf))) {
        nvbuf_surf = nullptr;
        try { 
            ReturnImage();
        } catch (...) {
        }
        throw std::runtime_error("Failed at NvBufSurfaceFromFd");
    }

    if (NvBufSurfaceMap(nvbuf_surf, 0, 0, NVBUF_MAP_READ_WRITE)) {
        try {
            ReturnImage();
        } catch (...) {

        }
        throw std::runtime_error("Failed to map plane 0");
    }

    NvBufSurfaceSyncForCpu(nvbuf_surf, 0, 0);
    image.strideY = nvbuf_surf->surfaceList[0].planeParams.pitch[0];
    image.planeY = (char*) nvbuf_surf->surfaceList[0].mappedAddr.addr[0];

    if (NvBufSurfaceMap(nvbuf_surf, 0, 1, NVBUF_MAP_READ_WRITE)) {
        try {
            ReturnImage();
        } catch (...) {

        }
        throw std::runtime_error("Failed to map plane 1");
    }

    NvBufSurfaceSyncForCpu(nvbuf_surf, 0, 1);
    image.strideUV = nvbuf_surf->surfaceList[0].planeParams.pitch[1];
    image.planeUV = (char*)nvbuf_surf->surfaceList[0].mappedAddr.addr[1];

    return &image;   
}

void Camera::ReturnImage()
{
    if (nvbuf_surf != nullptr) {
        if (image.planeY != nullptr) {
            NvBufSurfaceUnMap(nvbuf_surf, 0, 0);
        }
        if (image.planeUV != nullptr) {
            NvBufSurfaceUnMap(nvbuf_surf, 0, 1);
        }
    }
    
    image.planeY = nullptr;
    image.planeUV = nullptr;

	v4l2_buffer buffer = {};
	v4l2_plane planes[NV12_PLANES] = {};
	
	buffer.type = bufferType;
	buffer.memory = memoryType;
	buffer.length = NV12_PLANES;
	buffer.m.planes = planes;
	buffer.m.planes[0].m.fd = dmabuffers_fd[image.index];
	buffer.index = image.index;
	
	if(::v4l2_ioctl (fd, VIDIOC_QBUF, &buffer) == -1) {
		throw std::runtime_error("Failed to enqueue buffer");
	} 
}

uint32_t Camera::GetWidth()
{
	return width;
}

uint32_t Camera::GetHeight()
{
	return height;
}

std::vector<Control> Camera::GetControls()
{	
	InitializeControls(&controls);
	return controls;
}

void Camera::SetControl(Control control)
{
	v4l2isp::SetNewValue(control, fd);
}

void Camera::SetFrameRate(uint32_t framerate)
{
	SetSensorMode(sensorMode);
	
	struct v4l2_streamparm parms = {};
    parms.parm.capture.timeperframe.numerator = 1;
    parms.parm.capture.timeperframe.denominator = framerate;
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (::v4l2_ioctl(fd, VIDIOC_S_PARM, &parms) == -1) {
        std::cout << "Failed to set requested framerate. Default set by the library." << std::endl;
    }
    else {
		std::cout << "Framerate set to " << framerate << std::endl;
	}
}

FrameSize Camera::SelectFrameSize()
{
	return v4l2isp::SelectFrameSize(sensorModes, &sensorMode);
}

void Camera::GetSensorModes(int pipeID)
{
	fflush(stdout);
	
    std::string readBuffer;
	char buffer[4096] = {0};

	ssize_t bytesRead; 
    while((bytesRead = read(pipeID, buffer, sizeof(buffer))) > 0) {
        readBuffer.append(buffer, bytesRead);
    }
    if (readBuffer.empty()) {
		throw std::runtime_error("Failed to extract frame sizes.");
	} 

	std::vector<std::string> splitBuffer;
	std::string delimiter = "\n";
	size_t position = 0;
	
	while((position = readBuffer.find(delimiter)) != std::string::npos) {
		splitBuffer.push_back(readBuffer.substr(0, position));
		readBuffer.erase(0, position + delimiter.length());
	}

	auto newLines = std::remove_if(splitBuffer.begin(), splitBuffer.end(),
		[&] (std::string const& tempLine)
		{return  (tempLine.find("Opening") != std::string::npos) ||
                 (tempLine.find("Available") != std::string::npos) ||
                 (tempLine == "");});

	splitBuffer.erase(newLines, splitBuffer.end());
	
	delimiter = " ";
	position = 0;
	
	for (uint i = 0; i < splitBuffer.size(); i++) {
        std::istringstream lineStream(splitBuffer[i]);
        std::vector<std::string> tokens;
        std::string token;

        while (lineStream >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() < 8) {
            throw std::runtime_error("Unexpected sensor mode line");
        }

        SensorMode tempSensorMode {};
        tempSensorMode.index = i;

        try {
            tempSensorMode.frameSize.width  = std::stoi(tokens[1]);
            tempSensorMode.frameSize.height = std::stoi(tokens[3]);
            tempSensorMode.frameRate        = round(std::stof(tokens[7])); 
        } catch (const std::exception&) {
            throw std::runtime_error("Failed to parse sensor mode line " + splitBuffer[i]);
        }
        sensorModes.push_back(tempSensorMode);
	}
}

void Camera::SetSensorMode(uint32_t sensorMode)
{
		
	struct v4l2_ext_control control = {};
	struct v4l2_ext_controls ctrls = {};
		
	ctrls.count = 1;
	ctrls.controls = &control;
	ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	control.id = V4L2_CID_ARGUS_SENSOR_MODE;
	control.value = sensorMode;	
	
	if(::v4l2_ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) == -1) {
		throw std::runtime_error("Failed to set sensor mode.");
	}
}

void Camera::FreeBuffers()
{
    for (uint32_t i = 0; i < MAX_CAPTURE_BUFFFERS; ++i) {
        if (dmabuffers_fd[i] > 0) {
            NvBufSurface *surf = nullptr;
            if (NvBufSurfaceFromFd(dmabuffers_fd[i], (void**)&surf) == 0)
                NvBufSurfaceDestroy(surf);
            dmabuffers_fd[i] = 0;
        }
    }
    nvbuf_surf = nullptr;
    numOfBuffers = 0;
}

bool StdOut::Disable(int &pipeID)
{
    if (saved) {
        return false;
    }
    
    ::fflush(stdout);
    savedStdout = ::dup(STDOUT_FILENO);
    if (savedStdout == -1) {
        return false;
    }
    
	int tempPipe[2];
    
    if (pipe2(tempPipe, O_NONBLOCK) != 0) {
        return false;
    }

    pipeID = tempPipe[0];
    
	dup2(tempPipe[1], STDOUT_FILENO);   
	close(tempPipe[1]);
    
    saved = true;
    return true;
}

bool StdOut::Restore()
{
    if (!saved || savedStdout == -1) {
        return false;
    }
    ::fflush(stdout);
    auto result = ::dup2(savedStdout, STDOUT_FILENO);
    if (result == -1) {
        return false;
    }
    ::close(savedStdout);
    saved = false;
    return true;
}

}
