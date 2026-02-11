#pragma once

#include <cstdint>
#include <string>
#include <linux/media.h>

namespace media
{

class MediaEntity
{
    public:
        MediaEntity() {}
        MediaEntity(const struct media_entity_desc &entityDescriptor)
        : entityDescriptor(entityDescriptor) {}
        virtual ~MediaEntity() {}

        std::string GetName() const { return entityDescriptor.name; }
        uint32_t GetId() const { return entityDescriptor.id; }
        uint32_t GetType() const { return entityDescriptor.type; }
        uint32_t GetLinks() const { return entityDescriptor.links; }
        uint32_t GetMajor() const { return entityDescriptor.v4l.major; }
        uint32_t GetMinor() const { return entityDescriptor.v4l.minor; }

    private:
        struct media_entity_desc entityDescriptor;
};

}