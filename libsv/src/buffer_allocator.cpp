#include "buffer_allocator.hpp"

#include <cstring>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "easylogging++/easylogging++.h"

#include "buffer.hpp"

//TODO namespaces

BufferAllocator::BufferAllocator(int fd) : fd(fd), count(6) 
{
    bufferInfo = std::unique_ptr<BufferInfo>(new BufferInfo());
}

bool BufferAllocator::RequestBuffers()
{
    v4l2_requestbuffers bufrequest = {};
    bufrequest.type = bufferInfo->GetType();
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = count;
    
    bool result = ioctl(fd, VIDIOC_REQBUFS, &bufrequest) == 0;

    if (result == false) {
        LOG(ERROR) << "VIDIOC_REQBUFS failed";
        return result;
    }

    if (count != bufrequest.count) {
        LOG(WARNING) << "VIDIOC_REQBUFS granted fewer buffers than requested";
        count = bufrequest.count;
    }

    return result;
}

std::vector<Buffer> BufferAllocator::QueryBuffers()
{
    std::vector<Buffer> buffers;

    for (uint32_t i = 0; i < count; ++i) {

        v4l2_buffer info = bufferInfo->GetInfo(i);

        if (::ioctl(fd, VIDIOC_QUERYBUF, &info)){
            LOG(ERROR) << "VIDIOC_QUERYBUF failed";
            return std::vector<Buffer>();
        }

        uint32_t length = bufferInfo->GetLength(info);
        void* buffer_start = ::mmap(
            nullptr,
            length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            bufferInfo->GetOffset(info)
        );
        if (buffer_start == MAP_FAILED) {
            LOG(ERROR) << "mmap failed";
            return std::vector<Buffer>();
        }

        std::memset(buffer_start, 0, length);

        Buffer buffer;
        buffer.index = info.index;

        buffer.rawLength = length;
        buffer.rawData = (uint8_t *)buffer_start;

        buffers.push_back(buffer);
    }

    return buffers;
}

std::vector<Buffer> BufferAllocator::Allocate()
{
    if (RequestBuffers() == false) {
        LOG(ERROR) << "buffer allocation failed";
        return std::vector<Buffer>();
    }

    return QueryBuffers();
}

void BufferAllocator::Deallocate(std::vector<Buffer> &buffers)
{
    for (auto& buff : buffers) {

        if (buff.rawData != nullptr) {
            ::munmap(buff.rawData, buff.rawLength);
        }
    }

    v4l2_requestbuffers bufrequest = {};
    bufrequest.type = bufferInfo->GetType();
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 0;
    
    ::ioctl(fd, VIDIOC_REQBUFS, &bufrequest);
}


uint32_t BufferAllocator::GetBufferCount()
{
    return count;
}
        
void BufferAllocator::SetBufferCount(uint32_t count)
{
    BufferAllocator::count = count;
}

MplaneBufferAllocator::MplaneBufferAllocator(int fd) : BufferAllocator(fd)
{
    bufferInfo = std::unique_ptr<BufferInfo>(new MplaneBufferInfo());
}