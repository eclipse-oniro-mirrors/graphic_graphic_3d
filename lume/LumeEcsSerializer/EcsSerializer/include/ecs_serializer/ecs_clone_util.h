/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef API_ECS_SERIALIZER_ECSUTIL_H
#define API_ECS_SERIALIZER_ECSUTIL_H

#include <base/containers/unordered_map.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <ecs_serializer/namespace.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

inline void CloneComponent(CORE_NS::Entity srcEntity, const CORE_NS::IComponentManager& srcManager,
    CORE_NS::IEcs& dstEcs, CORE_NS::Entity dstEntity)
{
    auto* dstManager = dstEcs.GetComponentManager(srcManager.GetUid());
    if (dstManager) {
        // Get copy destiantion property handle.
        auto componentId = dstManager->GetComponentId(dstEntity);
        if (componentId == CORE_NS::IComponentManager::INVALID_COMPONENT_ID) {
            dstManager->Create(dstEntity);
            componentId = dstManager->GetComponentId(dstEntity);
        }
        BASE_ASSERT(componentId != CORE_NS::IComponentManager::INVALID_COMPONENT_ID);
        const auto* srcHandle = srcManager.GetData(srcEntity);
        if (srcHandle) {
            dstManager->SetData(dstEntity, *srcHandle);
        }
    }
}

inline void CloneComponents(
    CORE_NS::IEcs& srcEcs, CORE_NS::Entity srcEntity, CORE_NS::IEcs& dstEcs, CORE_NS::Entity dstEntity)
{
    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    srcEcs.GetComponents(srcEntity, managers);
    for (auto* srcManager : managers) {
        CloneComponent(srcEntity, *srcManager, dstEcs, dstEntity);
    }
}

inline CORE_NS::Entity CloneEntity(CORE_NS::IEcs& srcEcs, CORE_NS::Entity src, CORE_NS::IEcs& dstEcs)
{
    CORE_NS::Entity dst = dstEcs.GetEntityManager().Create();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

inline CORE_NS::EntityReference CloneEntityReference(CORE_NS::IEcs& srcEcs, CORE_NS::Entity src, CORE_NS::IEcs& dstEcs)
{
    CORE_NS::EntityReference dst = dstEcs.GetEntityManager().CreateReferenceCounted();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

inline void GatherEntityReferences(BASE_NS::vector<CORE_NS::Entity*>& entities,
    BASE_NS::vector<CORE_NS::EntityReference*>& entityReferences, const CORE_NS::Property& property,
    uintptr_t offset = 0)
{
    if (property.type == CORE_NS::PropertyType::ENTITY_T) {
        entities.emplace_back(reinterpret_cast<CORE_NS::Entity*>(offset));
    } else if (property.type == CORE_NS::PropertyType::ENTITY_REFERENCE_T) {
        entityReferences.emplace_back(reinterpret_cast<CORE_NS::EntityReference*>(offset));
    } else if (property.metaData.containerMethods) {
        auto& containerProperty = property.metaData.containerMethods->property;
        if (property.type.isArray) {
            // Array of properties.
            for (size_t i = 0; i < property.count; i++) {
                uintptr_t ptr = offset + i * containerProperty.size;
                GatherEntityReferences(entities, entityReferences, containerProperty, ptr);
            }
        } else {
            // This is a "non trivial container"
            // (So it needs to read the data and not just the metadata to figure out the data structure).
            const auto count = property.metaData.containerMethods->size(offset);
            for (size_t i = 0; i < count; i++) {
                uintptr_t ptr = property.metaData.containerMethods->get(offset, i);
                GatherEntityReferences(entities, entityReferences, containerProperty, ptr);
            }
        }
    } else if (!property.metaData.memberProperties.empty()) {
        // Custom type (struct). Process sub properties recursively.
        for (size_t i = 0; i < property.count; i++) {
            for (const auto& child : property.metaData.memberProperties) {
                GatherEntityReferences(entities, entityReferences, child, offset + child.offset);
            }
            offset += property.size / property.count;
        }
    }
}

inline void RewriteEntityReferences(
    CORE_NS::IEcs& ecs, CORE_NS::Entity entity, BASE_NS::unordered_map<CORE_NS::Entity, CORE_NS::Entity>& oldToNew)
{
    // Go through the entity properties and update any entity references to point to the ones pointed by the oldToNew
    // map.
    auto managers = ecs.GetComponentManagers();
    for (auto cm : managers) {
        if (auto id = cm->GetComponentId(entity); id != CORE_NS::IComponentManager::INVALID_COMPONENT_ID) {
            auto* data = cm->GetData(id);
            if (data) {
                // Find all entity references from this component.
                BASE_NS::vector<CORE_NS::Entity*> entities;
                BASE_NS::vector<CORE_NS::EntityReference*> entityRefs;
                uintptr_t offset = (uintptr_t)data->RLock();
                if (offset) {
                    for (const auto& property : data->Owner()->MetaData()) {
                        GatherEntityReferences(entities, entityRefs, property, offset + property.offset);
                    }

                    // Rewrite old entity values with new ones. Assuming that the memory locations are the same as in
                    // the RLock. NOTE: Keeping the read access open and we must not change any container sizes.
                    if (!entities.empty() || !entityRefs.empty()) {
                        data->WLock();
                        for (CORE_NS::Entity* current : entities) {
                            if (const auto it = oldToNew.find(*current); it != oldToNew.end()) {
                                *current = it->second;
                            }
                        }
                        for (CORE_NS::EntityReference* current : entityRefs) {
                            if (const auto it = oldToNew.find(*current); it != oldToNew.end()) {
                                *current = ecs.GetEntityManager().GetReferenceCounted(it->second);
                            }
                        }
                        data->WUnlock();
                    }
                }

                data->RUnlock();
            }
        }
    }
}

inline BASE_NS::vector<CORE_NS::Entity> CloneEntities(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::Entity> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::vector<CORE_NS::Entity> clonedEntities;
    clonedEntities.reserve(src.size());
    for (auto srcEntity : src) {
        clonedEntities.emplace_back(CloneEntity(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline BASE_NS::vector<CORE_NS::EntityReference> CloneEntityReferences(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::EntityReference> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::vector<CORE_NS::EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntityReference(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline BASE_NS::vector<CORE_NS::EntityReference> CloneEntitiesUpdateRefs(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::EntityReference> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::unordered_map<CORE_NS::Entity, CORE_NS::Entity> oldToNew;

    BASE_NS::vector<CORE_NS::EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntityReference(srcEcs, srcEntity, dstEcs));
        oldToNew[srcEntity] = clonedEntities.back();
    }

    for (const auto& entity : clonedEntities) {
        RewriteEntityReferences(dstEcs, entity, oldToNew);
    }
    return clonedEntities;
}

ECS_SERIALIZER_END_NAMESPACE()

#endif
