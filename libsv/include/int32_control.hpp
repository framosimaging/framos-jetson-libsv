#pragma once

#include "v4l2_control.hpp"
#include <linux/videodev2.h>

class Int32Control : public V4l2Control
{
    public:
        explicit Int32Control(int fd, v4l2_queryctrl control);
        virtual ~Int32Control() = default;
        int64_t Get() override;
        bool SetValue(int64_t val) override;

    private:
        bool IsArgusControl(std::string name);
};