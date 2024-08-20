#include <stdexcept>
#include <algorithm>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <iostream>
#include <cmath>
#include "libv4l2.h"
#include "v4l2isp/v4l2isp.hpp"
#include "v4l2isp/common.hpp"
#include "v4l2_nv_extensions.h"

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
 : name(node), 
  bufferType(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE), memoryType(V4L2_MEMORY_DMABUF),
  width(1920), height(1080), pixelformat(V4L2_PIX_FMT_NV12M),
  formatSet(false)
{
    /**
     * v4l2_open prints available modes when executed
     */
    StdOut stdOut;
    int pipeId = -1;
    stdOut.Disable(pipeId);
    fd = ::v4l2_open(node.c_str(), O_RDWR);
    sensorMode = -1;
  
    GetSensorModes(pipeId);
    
    stdOut.Restore();
    if (fd == -1) {
        throw std::runtime_error("Failed to open " + node);
    }


    v4l2_capability capabilities {};
    auto result = ::v4l2_ioctl(fd , VIDIOC_QUERYCAP, &capabilities);
    if (result == -1) {
        throw std::runtime_error("Failed to query capabilities");
    }
    if(!(capabilities.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
		throw std::runtime_error("Device does not support V4L2_CAP_VIDEO_CAPTURE_MPLANE");
	}
    driverName = reinterpret_cast<char*>(capabilities.card);

    nvbuf_surf = new NvBufSurface();
}


Camera::~Camera()
{
	::v4l2_close(fd);
    delete nvbuf_surf;
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
    auto result = ::v4l2_ioctl(fd, VIDIOC_S_FMT, &format);
    if (result == -1) {
        throw std::runtime_error("Failed to set format");
    }
    formatSet = true;
}

void Camera::StartStream()
{
	if (!formatSet) {
        SetFormat();
    }
	
	v4l2_requestbuffers reqbuffers = {};
	reqbuffers.count = MAX_CAPTURE_BUFFFERS;
    reqbuffers.memory = memoryType;
    reqbuffers.type = bufferType;
    
    if (v4l2_ioctl (fd, VIDIOC_REQBUFS, &reqbuffers)) {
        throw std::runtime_error("Failed to get dma buffers");
    }
	
    NvBufSurfaceAllocateParams cParams = {};

    if (reqbuffers.count) {
        for (uint32_t i = 0; i < reqbuffers.count; i++) {
            cParams.params.width = width;
            cParams.params.height = height;
            cParams.params.layout = NVBUF_LAYOUT_PITCH;
            cParams.params.colorFormat = NVBUF_COLOR_FORMAT_NV12;
            cParams.params.memType = NVBUF_MEM_SURFACE_ARRAY;
            cParams.memtag = NvBufSurfaceTag_CAMERA;
            
            if (NvBufSurfaceAllocate(&nvbuf_surf, 1, &cParams) != 0) {
                std::cout << "Failed to create NvBufSurface!\n";
            }
            nvbuf_surf->numFilled = 1;
            dmabuffers_fd[i] = nvbuf_surf->surfaceList[0].bufferDesc;
        }
    } else {
        for (uint32_t i = 0; i < MAX_CAPTURE_BUFFFERS; i++) {
            if (dmabuffers_fd[i]) {
                if (NvBufSurfaceFromFd((int)dmabuffers_fd[i], (void**)(&nvbuf_surf)) != 0) {
                    std::cout << "Failed to get NvBufSurface from FD\n";
                }
                if (NvBufSurfaceDestroy(nvbuf_surf) != 0) {
                    std::cout << "Failed to destroy NvBufSurface!\n";
                }
                dmabuffers_fd[i] = 0;
            }
        }
    }

	numOfBuffers = reqbuffers.count;

    struct v4l2_plane captureplanes[NV12_PLANES];
	for (uint32_t i = 0; i < numOfBuffers; ++i) {
			v4l2_buffer buffer = {};
            buffer.index = i;
            buffer.type = bufferType;
            buffer.memory = memoryType;
            buffer.m.planes = captureplanes;
            buffer.length = 2;
            buffer.m.planes[0].m.fd = dmabuffers_fd[i];

            if(v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buffer)) {
					throw std::runtime_error("Failed to query buffers");
			}
        }
	
	if(v4l2_ioctl (fd, VIDIOC_STREAMON, &bufferType)) {
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

		if(v4l2_ioctl (fd, VIDIOC_QBUF, &queue_cap_v4l2_buf)) {
			throw std::runtime_error("Failed to enqueue buffer");
		}
	} 		
}

void Camera::StopStream()
{
    auto result = ::v4l2_ioctl(fd, VIDIOC_STREAMOFF, &bufferType);
    if (result == -1) {
        throw std::runtime_error("Failed to stop stream");
    }

    struct v4l2_requestbuffers requestBuffers {};
    requestBuffers.type = bufferType;
    requestBuffers.memory = memoryType;
    requestBuffers.count = 0;
    result = ::v4l2_ioctl(fd, VIDIOC_REQBUFS, &requestBuffers);
    if (result == -1) {
        throw std::runtime_error("Failed to request 0 buffers");
    }
}

Image* Camera::GetImage()
{  
    v4l2_buffer v4l2_buf = {};
    v4l2_plane planes[NV12_PLANES] = {};
	
    v4l2_buf.m.planes = planes;
    v4l2_buf.length = NV12_PLANES;
	v4l2_buf.type = bufferType;
	v4l2_buf.memory = memoryType;

	auto result = v4l2_ioctl (fd, VIDIOC_DQBUF, &v4l2_buf);
	if(result) {
			throw std::runtime_error("Failed to dequeue buffer");
	}		

    int dmabuf_fd = v4l2_buf.m.planes[0].m.fd;

	if (dmabuf_fd <= 0) {
		throw std::runtime_error("dmabuf_fd <= 0");
	}
	
    int plane = 0;

    if (NvBufSurfaceFromFd(dmabuf_fd, (void**)(&nvbuf_surf))) {
        throw std::runtime_error("Failed at NvBufSurfaceFromFd!\n");
    }

    if (!NvBufSurfaceMap(nvbuf_surf, 0, plane, NVBUF_MAP_READ_WRITE)) {    
        dataY = (void*)nvbuf_surf->surfaceList[0].mappedAddr.addr[plane];
        NvBufSurfaceSyncForCpu(nvbuf_surf, 0, plane);
        image.strideY = nvbuf_surf->surfaceList[0].planeParams.pitch[plane];
        image.planeY = (char*)dataY;
    }

    plane = 1;
    if (!NvBufSurfaceMap(nvbuf_surf, 0, plane, NVBUF_MAP_READ_WRITE)) {    
        dataUV = (void*)nvbuf_surf->surfaceList[0].mappedAddr.addr[plane];
        NvBufSurfaceSyncForCpu(nvbuf_surf, 0, plane);
        image.strideUV = nvbuf_surf->surfaceList[0].planeParams.pitch[plane];
        image.planeUV = (char*)dataUV;
    }
 
    image.index = v4l2_buf.index; 
    
    return &image;   
}

void Camera::ReturnImage()
{
    NvBufSurfaceUnMap(nvbuf_surf, 0, 0);
    NvBufSurfaceUnMap(nvbuf_surf, 0, 1);
	v4l2_buffer buffer = {};
	v4l2_plane planes[NV12_PLANES] = {};
	
	buffer.type = bufferType;
	buffer.memory = memoryType;
	buffer.length = NV12_PLANES;
	buffer.m.planes = planes;
	buffer.m.planes[0].m.fd = dmabuffers_fd[image.index];
	buffer.index = image.index;
	
	if(v4l2_ioctl (fd, VIDIOC_QBUF, &buffer)) {
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

    if (v4l2_ioctl(fd, VIDIOC_S_PARM, &parms)) {
        std::cout << "Failed to set requested framerate. Default set by the library." << std::endl;
    }
    else {
		std::cout << "Framerate set to " << framerate << std::endl;
	}
}

FrameSize Camera::SelectFrameSize(){
	return v4l2isp::SelectFrameSize(sensorModes, &sensorMode);
}

void Camera::GetSensorModes(int pipeID)
{
	fflush(stdout);
	
	char buffer[4096] = {0};
	char headerString1[] = "Opening in BLOCKING MODE";
	char headerString2[] = "Available Sensor modes :";
	
	if(!read(pipeID, buffer, PIPE_BUF)){
		throw std::runtime_error("Failed to extract frame sizes.");
	} 
	
	std::string readBuffer(buffer);
	
	std::vector<std::string> splitBuffer;
	std::string delimiter = "\n";
	size_t position = 0;
	
	while((position = readBuffer.find(delimiter)) != std::string::npos) {
		splitBuffer.push_back(readBuffer.substr(0, position));
		readBuffer.erase(0, position + delimiter.length());
	} 
		
	auto newLines = std::remove_if(splitBuffer.begin(), splitBuffer.end(), 
		[&] (std::string const& tempLine) 
		{return  (tempLine == "") || (tempLine == headerString1) || (tempLine == headerString2);});
	splitBuffer.erase(newLines, splitBuffer.end());
	
	delimiter = " ";
	position = 0;
	
	for (uint i = 0; i < splitBuffer.size(); i++) {
		SensorMode tempSensorMode = {};
		//parse only Resolution and Framerate
		for (uint j = 0; j < 8; j++) {
			position = splitBuffer[i].find(delimiter);
			tempSensorMode.index = i;
			
			if (j == 1) {
				tempSensorMode.frameSize.width = std::stoi(splitBuffer[i].substr(0, position));
			}
			else if (j == 3) {
				tempSensorMode.frameSize.height = std::stoi(splitBuffer[i].substr(0, position));
			}
			else if (j == 7) {
				tempSensorMode.frameRate = round(std::stof(splitBuffer[i].substr(0, position)));
			}
			splitBuffer[i].erase(0, position + delimiter.length());
		}
		sensorModes.push_back(tempSensorMode);
	}
	

}

void Camera::SetSensorMode(uint32_t sensorMode){
		
	struct v4l2_ext_control control = {};
	struct v4l2_ext_controls ctrls = {};
		
	ctrls.count = 1;
	ctrls.controls = &control;
	ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
	control.id = V4L2_CID_ARGUS_SENSOR_MODE;
	control.value = sensorMode;	
	
	if(v4l2_ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls)){
		throw std::runtime_error("Failed to set sensor mode.");
	}
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
    
	if( pipe(tempPipe) != 0 ) {          
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
