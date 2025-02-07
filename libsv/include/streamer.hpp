#pragma once

#include <cstdint>
#include <memory>

#include "buffer_info.hpp"
#include "sv/sv.h"

struct DqStatus
{
    bool error;
    uint32_t bufferID;
    uint32_t sequence;
    struct Timestamp timestamp;
};

class Streamer
{
    public:
        Streamer(int fd);
        bool Start();
        bool Stop();
        bool AddToIncomingQueue(uint32_t index, bool* v4l2queuedStatus = nullptr);
        bool GetFromOutgoingQueue(DqStatus& status);
    
    private:
        int fd;

    protected:
        std::unique_ptr<BufferInfo> bufferInfo;
};

class MplaneStreamer : public Streamer
{
    public:
        MplaneStreamer(int fd);
};