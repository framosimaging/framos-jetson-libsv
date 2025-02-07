#pragma once

#include "v4l2_control.hpp"
#include <linux/videodev2.h>

class Int64Control final : public V4l2Control
{
    public:
        explicit Int64Control(int fd, v4l2_queryctrl control);
        int64_t Get() override;
        bool SetValue(int64_t val) override;
        void Update() override;

    private:
        void QueryControl();
};