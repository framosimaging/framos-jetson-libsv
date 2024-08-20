#include "image_pipeline.hpp"

namespace common
{

ImagePipeline::ImagePipeline(ICamera *camera) : camera(camera)
{

}

ImagePipeline::~ImagePipeline()
{
    
}

std::string ImagePipeline::GetName()
{
    if (name.empty()) {
        std::stringstream stream;
        stream << camera->GetName() << " - " << camera->GetDriverName();
        name = stream.str();
    }

    return name;
}

std::string ImagePipeline::GetCleanName()
{
    if (cleanName.empty()) {
        cleanName = GetName();
        cleanName.erase(std::remove_if(cleanName.begin(), cleanName.end(), isspace), cleanName.end());
        std::replace(cleanName.begin(), cleanName.end(), '/', '_');
        std::replace(cleanName.begin(), cleanName.end(), '-', '_');
        cleanName.erase(0, 1);
    } 

    return cleanName;
}

bool ImagePipeline::IsMaster()
{
    auto controls = camera->GetControlList();

    auto control = std::find_if(controls.begin(), controls.end(), 
        [](IControl *control) { 
            return std::string(control->GetName()) == "Operation Mode"; 
        }
    );

    if (control == controls.end()) {
        return true;
    } 

    if (!(*control)->IsMenu()) {
        return true;
    }

    auto menu = (*control)->GetMenuEntries();
    if (menu.empty()) {
        return true;
    }

    uint32_t index = (*control)->Get();
    if (menu.size() <= index) {
        return true;
    }

    return std::string(menu[index].name) == "Master Mode";
}

}