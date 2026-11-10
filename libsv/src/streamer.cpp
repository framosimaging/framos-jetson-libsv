#include "streamer.hpp"
#include <cstring>
#include <iostream>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>

Streamer::Streamer(int fd) : fd(fd) 
{
    bufferInfo = std::unique_ptr<BufferInfo>(new BufferInfo());
}

bool Streamer::Start()
{
    v4l2_buf_type type = bufferInfo->GetType();
    return ::ioctl(fd, VIDIOC_STREAMON, &type) == 0;
}

bool Streamer::Stop()
{
    v4l2_buf_type type = bufferInfo->GetType();
    return ::ioctl(fd, VIDIOC_STREAMOFF, &type) == 0;
}

bool Streamer::AddToIncomingQueue(uint32_t index, bool* v4l2queuedStatus)
{
    v4l2_buffer info = bufferInfo->GetInfo(index);

    auto res = ::ioctl(fd, VIDIOC_QBUF, &info) == 0;

    if (v4l2queuedStatus) {
        *v4l2queuedStatus = info.flags & V4L2_BUF_FLAG_QUEUED;
    }

    return res;
}

bool Streamer::GetFromOutgoingQueue(DqStatus& status)
{
    status = {};
    struct pollfd pfd = {};
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR;

    int pRes = poll(&pfd, 1, 100);
    if (pRes == 0) {
        status.hasBuffer = false;
        return true;
    }

    if (pRes < 0) {
        if (errno == EINTR) {
            status.hasBuffer = false;
            return true;
        }
        std::cerr << "poll() failed: " << strerror(errno) << std::endl;
        return false;
    }

    v4l2_buffer info = bufferInfo->GetInfo();
    if (::ioctl(fd, VIDIOC_DQBUF, &info) < 0) {
        status.errorCode = errno;

        if(errno == EAGAIN || errno == EINTR) {
            status.hasBuffer = false;
            return true;
        }
        return false;
    }

    status.hasBuffer = true;
    status.bufferID = info.index;
    status.sequence = info.sequence;
    status.error = info.flags & V4L2_BUF_FLAG_ERROR;
    status.timestamp.s = info.timestamp.tv_sec;
    status.timestamp.us = info.timestamp.tv_usec;
    return true;

}

MplaneStreamer::MplaneStreamer(int fd) : Streamer(fd)
{
    bufferInfo = std::unique_ptr<BufferInfo>(new MplaneBufferInfo());
}