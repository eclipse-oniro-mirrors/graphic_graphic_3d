/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_ECS_IENTITY_MANAGER_H
#define API_CORE_ECS_IENTITY_MANAGER_H

#include <cstddef>

#include <base/containers/generic_iterator.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** @ingroup group_ecs_ientitymanager */
/** IEntity manager */
class IEntityManager {
public:
    enum class EventType : uint8_t {
        /** Entity Created
         */
        CREATED,
        /** Entity was activated
         */
        ACTIVATED,
        /** Entity was de-activated
         */
        DEACTIVATED,
        /** Entity destroyed
         */
        DESTROYED
    };
    using IteratorValue = Entity;
    using Iterator = BASE_NS::IGenericIterator<const IEntityManager>;

    IEntityManager(const IEntityManager&) = delete;
    IEntityManager& operator=(const IEntityManager&) = delete;

    /** Create a new entity.
     * @return New entity which is not reference counted. It will exist until IEntityManager::Destroy is called.
     */
    virtual Entity Create() = 0;

    /** Create a new reference counted entity.
     * @return New reference counted entity. It will exist until there are no more references to it.
     */
    virtual EntityReference CreateReferenceCounted() = 0;

    /** Get a reference counter for the given entity. If the entity was originally create as not reference counted it
     * will now have a reference count and be destroyed once all references are gone.
     * @param entity Entity to reference count.
     * @return Reference counted entity.
     */
    virtual EntityReference GetReferenceCounted(Entity entity) = 0;

    /** Destroy entity.
     *  @param entity Entity what we want to destroy.
     */
    virtual void Destroy(const Entity entity) = 0;

    /** Destroy all entities
     */
    virtual void DestroyAllEntities() = 0;

    /** Checks if entity is alive.
     *  @param entity Entity which alive status we check.
     */
    virtual bool IsAlive(const Entity entity) const = 0;

    /** Sets entity active state, inactive entities count as not alive. Invalid entities ignored.
     *  @param entity Entity which active status we change
     *  @param state New active status
     */
    virtual void SetActive(const Entity entity, bool state) = 0;

    /** Get list of added entities since last call.
     */
    virtual BASE_NS::vector<Entity> GetAddedEntities() = 0;

    /** Get list of removed entities since last call.
     */
    virtual BASE_NS::vector<Entity> GetRemovedEntities() = 0;

    /** Get list of events.
        These are in actual event order, there can be more than one event per entity.
     */
    virtual BASE_NS::vector<BASE_NS::pair<Entity, EventType>> GetEvents() = 0;

    /** Returns generation ID of current entity manager state, this id changes whenever entities are created or
     * destroyed.
     */
    virtual uint32_t GetGenerationCounter() const = 0;

    enum class IteratorType : uint8_t {
        ALIVE,       /* iterate entities that are alive and active */
        DEACTIVATED, /* iterate entities that are deactivated */
    };
    virtual Iterator::Ptr Begin(IteratorType type = IteratorType::ALIVE) const = 0;
    virtual Iterator::Ptr End(IteratorType type = IteratorType::ALIVE) const = 0;

protected:
    IEntityManager() = default;
    virtual ~IEntityManager() = default;
};

using EntityIterator = BASE_NS::IIterator<CORE_NS::IEntityManager::Iterator>;

inline CORE_NS::EntityIterator begin(const CORE_NS::IEntityManager& manager)
{
    return CORE_NS::EntityIterator(manager.Begin(IEntityManager::IteratorType::ALIVE));
}

inline CORE_NS::EntityIterator end(const CORE_NS::IEntityManager& manager)
{
    return CORE_NS::EntityIterator(manager.End(IEntityManager::IteratorType::ALIVE));
}
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_IENTITY_MANAGER_H
