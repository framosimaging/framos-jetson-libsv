#include "mediapipeline.hpp"
#include "linux.hpp"

#include "easylogging++/easylogging++.h"

#include <linux/media.h>
#include <sys/ioctl.h>
#include <memory>

namespace media
{

MediaPipeline::MediaPipeline(std::string node)
{
    if (!linux::CheckFileDescriptor(node.c_str())) return;
    fd = linux::OpenFileDescriptor(node.c_str());

    struct media_entity_desc entityDescriptor = {};
    entityDescriptor.id = MEDIA_ENT_ID_FLAG_NEXT;
    while (::ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entityDescriptor) == 0) {
        entities.emplace_back(MediaEntity(entityDescriptor));
        entityDescriptor.id |= MEDIA_ENT_ID_FLAG_NEXT;
    }
}

MediaPipeline::~MediaPipeline() {}

std::vector<MediaEntity> MediaPipeline::FilterEntities(std::function<bool(const MediaEntity&)> predicate) const
{
    std::vector<MediaEntity> filtered;

    std::copy_if(entities.begin(), entities.end(), std::back_inserter(filtered),
        [&predicate](const MediaEntity& entity) { return predicate(entity); }
    );

    return filtered;
}

std::vector<MediaEntity> MediaPipeline::FilterEntitiesByName(std::string name) const
{
    return FilterEntities(
        [&name](const media::MediaEntity& e){
            return e.GetName() == name;
        }
    );
}

std::vector<MediaEntity> MediaPipeline::GetOutboundLinkedEntities(const MediaEntity& entity) const
{
    struct media_links_enum linksEnum = {};
    linksEnum.entity = entity.GetId();
    linksEnum.links = new struct media_link_desc[entity.GetLinks()];

    std::vector<MediaEntity> linkedEntities;

    if (::ioctl(fd, MEDIA_IOC_ENUM_LINKS, &linksEnum) == 0) {
        for (uint32_t i = 0; i < entity.GetLinks(); ++i) {
            auto filtered = FilterEntities(
                [&linksEnum](const MediaEntity &e){ 
                    return linksEnum.links->sink.entity == e.GetId(); 
                }
            );
            linkedEntities.insert(linkedEntities.end(), filtered.begin(), filtered.end());
        }
    } else {
        LOG(ERROR) << "MEDIA_IOC_ENUM_LINKS failed";
    }

    delete[] linksEnum.links;
    return linkedEntities;
}

bool MediaPipeline::EnableLink(const MediaEntity& sourceEntity, const MediaEntity& sinkEntity)
{
    struct media_link_desc linkDescription = {};

    linkDescription.source.entity = sourceEntity.GetId();
    linkDescription.source.index = 1;
    linkDescription.source.flags = MEDIA_PAD_FL_SOURCE;
    linkDescription.sink.entity = sinkEntity.GetId();
    linkDescription.sink.index = 0;
    linkDescription.sink.flags = MEDIA_PAD_FL_SINK;
    linkDescription.flags = MEDIA_LNK_FL_ENABLED;

    return ::ioctl(fd, MEDIA_IOC_SETUP_LINK, &linkDescription) == 0;
}

bool MediaPipeline::EnableLink(std::string sourceEntityName, std::string sinkEntityName)
{
    auto sourceEntities = FilterEntitiesByName(sourceEntityName);
    auto sinkEntities = FilterEntitiesByName(sinkEntityName);

    if (sourceEntities.empty()) {
        LogEnableLinkNotFound("source");
        return false;
    }

    if (sinkEntities.empty()) {
        LogEnableLinkNotFound("sink");
        return false;
    }

    auto sourceEntity = sourceEntities[0];
    auto sinkEntitiy = sinkEntities[0];

    if (sourceEntities.size() > 1)
        LogEnableLinkMultipleEntities(sourceEntityName, sourceEntity.GetId());

    if (sinkEntities.size() > 1)
        LogEnableLinkMultipleEntities(sinkEntityName, sinkEntitiy.GetId());

    return EnableLink(sourceEntity, sinkEntitiy);
}

void MediaPipeline::LogEnableLinkNotFound(std::string entityName)
{
    LOG(ERROR) << "failed to enable media entity link - " << entityName << " not found";
}

void MediaPipeline::LogEnableLinkMultipleEntities(std::string entitiyName, uint32_t selectedId)
{
    LOG(WARNING) << "multiple entities with name " << entitiyName << " found, using one with ID" << selectedId;
}

}