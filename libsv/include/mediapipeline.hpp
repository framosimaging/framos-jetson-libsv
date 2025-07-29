#pragma once

#include "mediaentity.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace media 
{

class MediaPipeline 
{
    public:
        explicit MediaPipeline(std::string node);
        ~MediaPipeline();
        
        std::vector<MediaEntity> FilterEntities(std::function<bool(const MediaEntity&)> predicate) const;
        std::vector<MediaEntity> FilterEntitiesByName(std::string name) const;
        std::vector<MediaEntity> GetOutboundLinkedEntities(const MediaEntity& entity) const;
        bool EnableLink(std::string sourceEntityName, std::string sinkEntityName);

    private:
    
        int32_t fd;
        std::vector<MediaEntity> entities;

        bool EnableLink(const MediaEntity& sourceEntity, const MediaEntity& sinkEntity);
        void LogEnableLinkNotFound(std::string entityName);
        void LogEnableLinkMultipleEntities(std::string entitiyName, uint32_t selectedId);
};

}