#include "buffer_info.hpp"

BufferInfo::BufferInfo() {}

v4l2_buf_type BufferInfo::GetType()
{
    return V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

v4l2_buffer BufferInfo::GetInfo(uint32_t index)
{
    v4l2_buffer info = {};

    info.type = GetType();
    info.memory = V4L2_MEMORY_MMAP;
    info.index = index;

    return info;
}

uint32_t BufferInfo::GetLength(v4l2_buffer &info)
{
    return info.length;
}

uint32_t BufferInfo::GetOffset(v4l2_buffer &info)
{
    return info.m.offset;
}

MplaneBufferInfo::MplaneBufferInfo() : BufferInfo() {}

v4l2_buf_type MplaneBufferInfo::GetType()
{
    return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
}

v4l2_buffer MplaneBufferInfo::GetInfo(uint32_t index)
{
    v4l2_buffer info = BufferInfo::GetInfo(index);

    info.type = GetType();
    v4l2_plane planes[VIDEO_MAX_PLANES] = {};
    info.m.planes = planes;
    info.length = VIDEO_MAX_PLANES;

    return info;
}

uint32_t MplaneBufferInfo::GetLength(v4l2_buffer &info)
{
    return info.m.planes[0].length;
}

uint32_t MplaneBufferInfo::GetOffset(v4l2_buffer &info)
{
    return info.m.planes[0].m.mem_offset;
}