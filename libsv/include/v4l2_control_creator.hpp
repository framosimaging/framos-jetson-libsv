#pragma once

#include "v4l2_control.hpp"
#include <vector>
#include <memory>

class V4l2ControlCreator
{
    public:
        std::vector<std::unique_ptr<V4l2Control>> CreateControls(int fd);
};