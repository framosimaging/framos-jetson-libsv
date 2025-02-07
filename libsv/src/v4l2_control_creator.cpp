#include "v4l2_control_creator.hpp"
#include "int32_control.hpp"
#include "int64_control.hpp"
#include "menu_control.hpp"
#include "easylogging++/easylogging++.h"
#include <sys/ioctl.h>
#include <linux/videodev2.h>

std::vector<std::unique_ptr<V4l2Control>> V4l2ControlCreator::CreateControls(int fd)
{
    std::vector<std::unique_ptr<V4l2Control>> v4l2Controls;

    v4l2_queryctrl qctrl = {};
    qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while(::ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0)
    {
        switch(qctrl.type) {
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BOOLEAN:
        case V4L2_CTRL_TYPE_BUTTON:
            v4l2Controls.push_back(std::unique_ptr<Int32Control>(new Int32Control(fd, qctrl)));
            break;
        case V4L2_CTRL_TYPE_INTEGER64:
            v4l2Controls.push_back(std::unique_ptr<Int64Control>(new Int64Control(fd, qctrl)));
            break;
        case V4L2_CTRL_TYPE_MENU:
        case V4L2_CTRL_TYPE_INTEGER_MENU:
            v4l2Controls.push_back(std::unique_ptr<MenuControl>(new MenuControl(fd, qctrl)));
            break;
        default:
            LOG(WARNING) << "Control " << qctrl.name << " is of unsupported type (" << qctrl.type << " from enum v4l2_ctrl_type) and will not be available";
        }
        qctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    return v4l2Controls;
}