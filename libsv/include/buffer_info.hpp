#pragma once

#include <linux/videodev2.h>
#include <cinttypes>

class BufferInfo
{
    public:
        BufferInfo();
        virtual v4l2_buf_type GetType();
        virtual v4l2_buffer GetInfo(uint32_t index = 0);
        virtual uint32_t GetLength(v4l2_buffer &info);
        virtual uint32_t GetOffset(v4l2_buffer &info);

    private:
        v4l2_buf_type type;
};

class MplaneBufferInfo : public BufferInfo
{
    public:
        MplaneBufferInfo();
        v4l2_buf_type GetType() override;
        v4l2_buffer GetInfo(uint32_t index = 0) override;
        uint32_t GetLength(v4l2_buffer &info) override;
        uint32_t GetOffset(v4l2_buffer &info) override;
};