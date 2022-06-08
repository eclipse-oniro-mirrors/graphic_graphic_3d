/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "component_query.h"

#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::move;

ComponentQuery::~ComponentQuery()
{
    UnregisterEcsListeners();
}

bool ComponentQuery::ResultRow::IsValidComponentId(size_t index) const
{
    if (index < components.size()) {
        return components[index] != IComponentManager::INVALID_COMPONENT_ID;
    }
    return false;
}

void ComponentQuery::SetupQuery(
    const IComponentManager& baseComponentSet, const array_view<const Operation> operations, bool enableEntityLookup)
{
    const size_t componentCount = operations.size() + 1;
    enableLookup_ = enableEntityLookup;

    result_.clear();
    valid_ = false;

    // Unregistering any old listeners because the operations might not use the same managers.
    UnregisterEcsListeners();

    managers_.clear();
    managers_.reserve(componentCount);
    operationMethods_.clear();
    operationMethods_.reserve(componentCount);

    managers_.emplace_back(const_cast<IComponentManager*>(&baseComponentSet));
    operationMethods_.emplace_back(Operation::REQUIRE);
    for (auto& operation : operations) {
        managers_.emplace_back(const_cast<IComponentManager*>(&operation.target));
        CORE_ASSERT(managers_.back());
        operationMethods_.emplace_back(operation.method);
    }

    if (enableListeners_) {
        RegisterEcsListeners();
    }
}

bool ComponentQuery::Execute()
{
    if (enableListeners_ && valid_) {
        // No changes detected since previous execute.
        return false;
    }
    if (managers_.empty()) {
        // Query setup not done.
        return false;
    }

    const IComponentManager& baseComponentSet = *managers_[0];

    result_.clear();
    result_.reserve(baseComponentSet.GetComponentCount());

    auto& em = baseComponentSet.GetEcs().GetEntityManager();
    for (IComponentManager::ComponentId id = 0; id < baseComponentSet.GetComponentCount(); ++id) {
        if (const Entity entity = baseComponentSet.GetEntity(id); em.IsAlive(entity)) {
            const size_t managerCount = managers_.size();

            ResultRow row { entity, managerCount };
            auto componentIt = row.components.begin();
            *componentIt++ = id;

            bool valid = true;

            // NOTE: starting from index 1 that is the first manager after the base component set.
            for (size_t i = 1; valid && (i < managerCount); ++i, ++componentIt) {
                const auto& manager = *managers_[i];
                const auto componentId = manager.GetComponentId(entity);
                *componentIt = componentId;

                switch (operationMethods_[i]) {
                    case Operation::REQUIRE: {
                        // for required components ID must be valid
                        valid = (componentId != IComponentManager::INVALID_COMPONENT_ID);
                        break;
                    }

                    case Operation::OPTIONAL: {
                        // for optional ID doesn't matter
                        break;
                    }

                    default: {
                        valid = false;
                    }
                }
            }

            if (valid) {
                if (enableLookup_) {
                    mapping_[entity] = result_.size();
                }

                result_.emplace_back(move(row));
            }
        }
    }

    valid_ = true;
    return true;
}

void ComponentQuery::Execute(
    const IComponentManager& baseComponentSet, const array_view<const Operation> operations, bool enableEntityLookup)
{
    SetupQuery(baseComponentSet, operations, enableEntityLookup);
    Execute();
}

void ComponentQuery::SetEcsListenersEnabled(bool enableListeners)
{
    enableListeners_ = enableListeners;
    if (enableListeners_) {
        RegisterEcsListeners();
    } else {
        UnregisterEcsListeners();
    }
}

bool ComponentQuery::IsValid() const
{
    return valid_;
}

array_view<const ComponentQuery::ResultRow> ComponentQuery::GetResults() const
{
    return { result_.data(), result_.size() };
}

const ComponentQuery::ResultRow* ComponentQuery::FindResultRow(Entity entity) const
{
    const auto it = mapping_.find(entity);
    if (it != mapping_.end() && it->second < result_.size()) {
        return &(result_[it->second]);
    }

    return nullptr;
}

void ComponentQuery::RegisterEcsListeners()
{
    if (!registered_ && !managers_.empty()) {
        // Listen to changes in managers so the result can be automatically invalidated.
        ecs_ = &managers_[0]->GetEcs();
        for (auto& manager : managers_) {
            ecs_->AddListener(*manager, *this);
        }
        ecs_->AddListener(static_cast<IEcs::EntityListener&>(*this));
        registered_ = true;
    }
}

void ComponentQuery::UnregisterEcsListeners()
{
    if (registered_ && !managers_.empty() && ecs_) {
        for (auto& manager : managers_) {
            ecs_->RemoveListener(*manager, *this);
        }
        ecs_->RemoveListener(static_cast<IEcs::EntityListener&>(*this));
        registered_ = false;
    }
}

void ComponentQuery::OnEntityEvent(const IEcs::EntityListener::EventType type, const array_view<const Entity> entities)
{
    if (!valid_) {
        // Listener is only used to invalidate the quety. If the query is already invalid -> no need to check anything.
        return;
    }
    if (type == IEcs::EntityListener::EventType::ACTIVATED || type == IEcs::EntityListener::EventType::DEACTIVATED) {
        const auto managerCount = managers_.size();
        for (const auto& entity : entities) {
            // We are only interested in entities that have all the required managers.
            bool isRelevantEntity = true;
            for (size_t i = 0; i < managerCount; ++i) {
                if (operationMethods_[i] == Operation::OPTIONAL) {
                    continue;
                }
                if (!managers_[i]->HasComponent(entity)) {
                    // This entity is missing a required manager -> irrelevant.
                    isRelevantEntity = false;
                    break;
                }
            }

            if (isRelevantEntity) {
                // Marking this query as invalid. No need to iterate entities further.
                valid_ = false;
                return;
            }
        }
    }
}

void ComponentQuery::OnComponentEvent(const IEcs::ComponentListener::EventType type,
    const IComponentManager& componentManager, const array_view<const Entity> entities)
{
    // We only get events from relevant managers. If they have new or deleted components, the query is no longer valid.
    if (type == IEcs::ComponentListener::EventType::CREATED || type == IEcs::ComponentListener::EventType::DESTROYED) {
        valid_ = false;
    }
}
CORE_END_NAMESPACE()
