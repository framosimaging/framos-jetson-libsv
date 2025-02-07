#pragma once

#include <vector>
#include <cstdint>
#include <memory>

#include "buffer_info.hpp"

struct Buffer;

class BufferAllocator
{
    public:
        explicit BufferAllocator(int fd);
        std::vector<Buffer> Allocate();
        void Deallocate(std::vector<Buffer> &buffers);
        uint32_t GetBufferCount();
        void SetBufferCount(uint32_t count);

    private:
        int fd;
        uint32_t count;

        bool RequestBuffers();
        std::vector<Buffer> QueryBuffers();

    protected:
        std::unique_ptr<BufferInfo> bufferInfo;
};

class MplaneBufferAllocator : public BufferAllocator
{
    public:
        explicit MplaneBufferAllocator(int fd);
};