#include <algorithm>
#include <iomanip>
#include "v4l2isp/common.hpp"
#include "libv4l2.h"
#include "common_cpp/common.hpp"
#include "v4l2_nv_extensions.h"

namespace v4l2isp
{
	struct RangeFloat{
		float min;
		float max;
	};

	std::string DecodeFourCc(uint32_t format)
	{
		std::string result(4, ' ');
		
		result[0] = format & 0xFF;
		result[1] = (format >> 8) & 0xFF;
		result[2] = (format >> 16) & 0xFF;
		result[3] = (format >> 24) & 0xFF;
		
		return result;
	}
	
	std::string GetPixelFormatDescription(uint32_t format)
	{
		std::stringstream description;
		description << "12bit (" << DecodeFourCc(format) << ")";
	
		return description.str();
	}
	
	void InitializeControls(std::vector<Control> *controlsList)
	{
		//Controls cannot be queried via VIDIOC_QUERYCTRL
		controlsList->reserve(3);
		Control tempControl;
		
		tempControl.name = "Auto Exposure time range";
		tempControl.id = V4L2_CID_ARGUS_EXPOSURE_TIME_RANGE;
		tempControl.min = 34000;
		tempControl.max  = 358733000;
		controlsList->push_back(tempControl);
		
		tempControl.name = "Gain range";
		tempControl.id = V4L2_CID_ARGUS_GAIN_RANGE;
		tempControl.min = 1;
		tempControl.max = 16;
		controlsList->push_back(tempControl);
		
		tempControl.name = "ISP digital gain range";
		tempControl.id = V4L2_CID_ARGUS_ISP_DIGITAL_GAIN_RANGE;
		tempControl.min = 1;
		tempControl.max = 256;
		controlsList->push_back(tempControl);	
	}
	
	void SelectNewValue(Control *control)
	{
		std::cout << SetControlString("Choose a " + control->name + " ", control->min, control->max) << std::endl;
		std::cout << "Please enter values in form: min, max" << std::endl << ">" ;
		ParseInput(control->min, control->max);
	}
	
	void SetNewValue(Control updatedControl, int fd)
	{
		struct v4l2_ext_control control = {};
		struct v4l2_ext_controls ctrls = {};
		
		RangeFloat rangeFloat = {};
		
		ctrls.count = 1;
		ctrls.controls = &control;
		ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
		control.id = updatedControl.id;
		
		rangeFloat.min = (float)updatedControl.min;
		rangeFloat.max = (float)updatedControl.max;
		control.string = (char *)&rangeFloat;		
				
		if(v4l2_ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls)){
			throw std::runtime_error("Failed to set control " + std::string(updatedControl.name));
		} else {
			std::cout << SetControlString(updatedControl.name, updatedControl.min, updatedControl.max, std::string(" set to ")) << std::endl << std::endl;
		}
	}
	
	std::string SetControlString(std::string ctrlName, uint32_t min, uint32_t max, std::string opt){
		return ctrlName + opt +  "[" + std::to_string(min) +  ", " + std::to_string(max) + "]";
	}
	
	std::shared_ptr<Camera> SelectCamera(const std::vector<std::string> &cameras)
	{
		std::cout << "Choose a camera [0]\n";
		uint32_t i = 0;
		for (const auto &camera : cameras) {
			std::cout << i++ << " - " << camera << std::endl;
		}
		int32_t index = common::QueryNumber(0, cameras.size() - 1, 0);
		return std::make_shared<Camera>( cameras[index]) ;
	}
	
	FrameSize SelectFrameSize(const std::vector<SensorMode>& sensorModes, uint32_t *sensorMode)
	{	
		std::cout << "Choose a frame size [0]\n";
		for(auto mode : sensorModes) {
			std::cout << mode.index << " - " << mode.frameSize.width << " x " << std::left << std::setfill(' ') << std::setw(4) << mode.frameSize.height  <<  "   (" << mode.frameRate << " fps max)" << std::endl;
		}
		std::cout << std::endl;
		
		auto index = common::QueryNumber(0, sensorModes.size() - 1, 0);
		*sensorMode = index;
		
		return sensorModes[index].frameSize;
		
	}
	
	uint32_t SelectPixelformat(const std::vector<uint32_t>& pixelformats)
	{
		std::cout << "Choose a pixel format [0]\n";
		for (uint32_t i = 0; i < pixelformats.size(); ++i) {
            std::cout << i << " - " << GetPixelFormatDescription(pixelformats[i]) << std::endl;
        }

        auto index = common::QueryNumber(0, pixelformats.size() - 1, 0);
		return pixelformats[index];
	}
	
	uint32_t SelectFrameRate()
	{	
		std::cout << "Chose a frame rate [30]" << std::endl;
		std::cout << "Max frame rate for selected frame size is listed above" << std::endl;
		return common::QueryNumber((uint32_t)1, (uint32_t)1000, 30);
	}     
 
	int32_t SelectValue(std::string name, int32_t minValue, int32_t maxValue, int32_t defaultValue)
    {
        std::cout << "Enter " << name << " [" << defaultValue << "]" << std::endl;
        std::cout << "Min: " << minValue << std::endl;
        std::cout << "Max: " << maxValue << std::endl;

        return common::QueryNumber(minValue, maxValue, defaultValue, true);
    }  
	
	void ExitOnEnter()
	{
		common::ExitOnEnter();
	}

	void ParseInput(uint32_t &min, uint32_t &max)
	{
	    std::string delimiter = ", ";
	    std::string selection = "";
	    size_t pos = 0;
	    uint32_t tempMin = 0;
	    uint32_t tempMax = 0;
	    bool inputValid = false;

	    do {
            std::getline(std::cin, selection);
            if (selection == "") {
            	return;
            }

            try {
                pos = selection.find(delimiter);

                if (pos == std::string::npos) {
                    std::cout << "Invalid input!\n" << "Please try again." << std::endl << ">";
                    continue;
                }

                else {
                    std::string minS = selection.substr(0, pos);
                    std::string maxS = selection.erase(0, pos + delimiter.length());
			
                    if (!std::all_of(minS.begin(), minS.end(), [](unsigned char c) {return std::isdigit(c); }) ||
                        !std::all_of(maxS.begin(), maxS.end(), [](unsigned char c) {return std::isdigit(c); })) {
                        std::cout << "Invalid input!\n" << "Please try again." << std::endl << ">";
                        continue;
                    }
                    else {
                        tempMin = std::stoi(minS);
                        tempMax = std::stoi(maxS);
                        inputValid = true;
                    }
                }

                if (inputValid && SelectedValuesValid(min, max, tempMin, tempMax)) {
                    min = tempMin;
                    max = tempMax;
                }
                else {
                    std::cout << "Invalid range!\n" << "Please try again." << std::endl << ">";
                    inputValid = false;
                    }
                }
                catch (...) {
                    std::cout << "Invalid input!\n" << "Please try again." << std::endl << ">";
                    inputValid = false;
                }
        } while (!inputValid);
    }
	
    bool SelectedValuesValid(const uint32_t limitMin, const uint32_t limitMax, const uint32_t min, const uint32_t max)
    {
        if ((min >= limitMin && max <= limitMax) && (min <= max)) {
            return true;
        } else {
            return false;
        }
    }

}
