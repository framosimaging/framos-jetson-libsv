#pragma once

#include "generic_control.hpp"
#include <linux/videodev2.h>

class V4l2Control : public GenericControl
{
    public:
        explicit V4l2Control(int fd, v4l2_queryctrl control) 
        : GenericControl(control.id, std::string((char*)control.name), control.minimum, control.maximum, control.step, control.default_value),
        fd(fd) { }
        explicit V4l2Control(int fd, uint32_t id, std::string name) : GenericControl(id, name, 0, 0, 0, 0), fd(fd) { }
        virtual ~V4l2Control() = default;
    
    protected:
        const int fd;
};