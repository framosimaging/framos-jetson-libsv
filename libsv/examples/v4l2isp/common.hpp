#pragma once

#include "v4l2isp.hpp"

namespace v4l2isp
{
	std::string DecodeFourCc(uint32_t format);
	
	std::string GetPixelFormatDescription(uint32_t format);

	void InitializeControls(std::vector<Control> *controlsList);
		
	void SelectNewValue(Control *control);
	
	void SetNewValue(Control updatedControl, int fd);
	
	std::string SetControlString(std::string ctrlName, uint32_t min, uint32_t max, std::string opt = "");
	
    std::shared_ptr<Camera> SelectCamera(const std::vector<std::string> &cameras);
    
	FrameSize SelectFrameSize(const std::vector<SensorMode>& sensorModes, uint32_t *sensorMode);
	
	uint32_t SelectPixelformat(const std::vector<uint32_t>& pixelformats);
	
	uint32_t SelectFrameRate();

	int32_t SelectValue(std::string name, int32_t minValue, int32_t maxValue, int32_t defaultValue);

	void ExitOnEnter();
	
	void ParseInput(uint32_t &min, uint32_t &max);
	
	bool SelectedValuesValid(const uint32_t limitMin, const uint32_t limitMax, const uint32_t min, const uint32_t max);
	 
}
