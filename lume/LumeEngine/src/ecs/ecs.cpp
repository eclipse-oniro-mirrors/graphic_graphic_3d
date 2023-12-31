/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <atomic>

#include <base/containers/unordered_map.h>
#include <core/ecs/intf_ecs.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/threading/intf_thread_pool.h>

#include "ecs/entity_manager.h"

CORE_BEGIN_NAMESPACE()
namespace {
using BASE_NS::array_view;
using BASE_NS::pair;
using BASE_NS::Uid;
using BASE_NS::unique_ptr;
using BASE_NS::unordered_map;
using BASE_NS::vector;

class Ecs final : public IEcs, IPluginRegister::ITypeInfoListener {
public:
    explicit Ecs(IClassFactory&, const IThreadPool::Ptr& threadPool);
    ~Ecs() override;
    IEntityManager& GetEntityManager() override;
    void GetComponents(Entity entity, vector<IComponentManager*>& result) override;
    vector<ISystem*> GetSystems() const override;
    ISystem* GetSystem(const Uid& uid) const override;
    vector<IComponentManager*> GetComponentManagers() const override;
    IComponentManager* GetComponentManager(const Uid& uid) const override;
    Entity CloneEntity(const Entity entity) override;
    void ProcessEvents() override;

    void Initialize() override;
    bool Update(uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    IComponentManager* CreateComponentManager(const ComponentManagerTypeInfo& componentManagerTypeInfo) override;
    ISystem* CreateSystem(const SystemTypeInfo& systemInfo) override;

    void AddListener(EntityListener& listener) override;
    void RemoveListener(EntityListener& listener) override;
    void AddListener(ComponentListener& listener) override;
    void RemoveListener(ComponentListener& listener) override;
    void AddListener(IComponentManager& manager, ComponentListener& listener) override;
    void RemoveListener(IComponentManager& manager, ComponentListener& listener) override;

    void RequestRender() override;
    void SetRenderMode(RenderMode renderMode) override;
    RenderMode GetRenderMode() override;

    bool NeedRender() const override;

    IClassFactory& GetClassFactory() const override;

    const IThreadPool::Ptr& GetThreadPool() const override;

    float GetTimeScale() const override;
    void SetTimeScale(float scale) override;

    void Ref() noexcept override;
    void Unref() noexcept override;

protected:
    using SystemPtr = unique_ptr<ISystem, SystemTypeInfo::DestroySystemFn>;
    using ManagerPtr = unique_ptr<IComponentManager, ComponentManagerTypeInfo::DestroyComponentManagerFn>;

    // IPluginRegister::ITypeInfoListener
    void OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos) override;

    void ProcessAdded();
    void ProcessModified();
    void ProcessRemoved();

    IThreadPool::Ptr threadPool_;

    // for storing systems and component managers in creation order
    vector<SystemPtr> systemOrder_;
    vector<ManagerPtr> managerOrder_;
    // for finding systems and component managers with UID
    unordered_map<Uid, ISystem*> systems_;
    unordered_map<Uid, IComponentManager*> managers_;

    vector<EntityListener*> entityListeners_;
    vector<ComponentListener*> componentListeners_;
    unordered_map<IComponentManager*, vector<ComponentListener*>> componentManagerListeners_;

    bool needRender_ { false };
    bool renderRequested_ { false };
    RenderMode renderMode_ { RENDER_ALWAYS };

    IClassFactory& pluginRegistry_;
    EntityManager entityManager_;

    vector<pair<PluginToken, const IEcsPlugin*>> plugins_;
    float timeScale_ { 1.f };
    std::atomic<int32_t> refcnt_ { 0 };
};

template<typename ListIterator, typename ValueType>
auto find(ListIterator begin, ListIterator end, const ValueType& value)
{
    for (auto b = begin; b != end; ++b) {
        if (*b == value) {
            return b;
        }
    }
    return end;
}

template<typename ListType, typename ValueType>
auto find(ListType& list, const ValueType& value)
{
    return find(list.begin(), list.end(), value);
}

void Ecs::AddListener(EntityListener& listener)
{
    if (find(entityListeners_, &listener) != entityListeners_.end()) {
        // already added.
        return;
    }
    entityListeners_.push_back(&listener);
}

void Ecs::RemoveListener(EntityListener& listener)
{
    if (auto it = find(entityListeners_, &listener); it != entityListeners_.end()) {
        // Setting the listener to null instead of removing. This allows removing listeners from a listener callback.
        *it = nullptr;
        return;
    }
}

void Ecs::AddListener(ComponentListener& listener)
{
    if (find(componentListeners_, &listener) != componentListeners_.end()) {
        // already added.
        return;
    }
    componentListeners_.push_back(&listener);
}

void Ecs::RemoveListener(ComponentListener& listener)
{
    if (auto it = find(componentListeners_, &listener); it != componentListeners_.end()) {
        *it = nullptr;
        return;
    }
}

void Ecs::AddListener(IComponentManager& manager, ComponentListener& listener)
{
    auto list = componentManagerListeners_.find(&manager);
    if (list != componentManagerListeners_.end()) {
        if (auto it = find(list->second, &listener); it != list->second.end()) {
            return;
        }
        list->second.push_back(&listener);
        return;
    }
    componentManagerListeners_[&manager].push_back(&listener);
}

void Ecs::RemoveListener(IComponentManager& manager, ComponentListener& listener)
{
    auto list = componentManagerListeners_.find(&manager);
    if (list == componentManagerListeners_.end()) {
        return;
    }
    if (auto it = find(list->second, &listener); it != list->second.end()) {
        *it = nullptr;
        return;
    }
}

IComponentManager* Ecs::CreateComponentManager(const ComponentManagerTypeInfo& componentManagerTypeInfo)
{
    IComponentManager* manager = nullptr;
    if (componentManagerTypeInfo.createManager) {
        manager = GetComponentManager(componentManagerTypeInfo.UID);
        if (manager) {
            CORE_LOG_W("Duplicate component manager creation, returning existing instance");
        } else {
            manager = componentManagerTypeInfo.createManager(*this);
            if (manager) {
                managers_.insert({ componentManagerTypeInfo.uid, manager });
                managerOrder_.emplace_back(manager, componentManagerTypeInfo.destroyManager);
            }
        }
    }
    return manager;
}

ISystem* Ecs::CreateSystem(const SystemTypeInfo& systemInfo)
{
    ISystem* system = nullptr;
    if (systemInfo.createSystem) {
        system = GetSystem(systemInfo.UID);
        if (system) {
            CORE_LOG_W("Duplicate system creation, returning existing instance");
        } else {
            system = systemInfo.createSystem(*this);
            if (system) {
                systems_.insert({ systemInfo.uid, system });
                systemOrder_.emplace_back(system, systemInfo.destroySystem);
            }
        }
    }
    return system;
}

Ecs::Ecs(IClassFactory& registry, const IThreadPool::Ptr& threadPool)
    : threadPool_(threadPool), pluginRegistry_(registry)
{
    for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(IEcsPlugin::UID)) {
        if (auto ecsPlugin = static_cast<const IEcsPlugin*>(info); ecsPlugin && ecsPlugin->createPlugin) {
            auto token = ecsPlugin->createPlugin(*this);
            plugins_.push_back({ token, ecsPlugin });
        }
    }
    GetPluginRegister().AddListener(*this);
}

Ecs::~Ecs()
{
    GetPluginRegister().RemoveListener(*this);

    Uninitialize();
    managerOrder_.clear();
    systemOrder_.clear();

    for (auto& plugin : plugins_) {
        if (plugin.second->destroyPlugin) {
            plugin.second->destroyPlugin(plugin.first);
        }
    }
}

IClassFactory& Ecs::GetClassFactory() const
{
    return pluginRegistry_;
}

IEntityManager& Ecs::GetEntityManager()
{
    return entityManager_;
}

void Ecs::GetComponents(Entity entity, vector<IComponentManager*>& result)
{
    result.clear();
    result.reserve(managers_.size());
    for (auto& m : managerOrder_) {
        if (m->HasComponent(entity)) {
            result.push_back(m.get());
        }
    }
}

vector<ISystem*> Ecs::GetSystems() const
{
    vector<ISystem*> result;
    result.reserve(systemOrder_.size());
    for (auto& t : systemOrder_) {
        result.push_back(t.get());
    }
    return result;
}

ISystem* Ecs::GetSystem(const Uid& uid) const
{
    if (auto pos = systems_.find(uid); pos != systems_.end()) {
        return pos->second;
    }
    return nullptr;
}

vector<IComponentManager*> Ecs::GetComponentManagers() const
{
    vector<IComponentManager*> result;
    result.reserve(managerOrder_.size());
    for (auto& t : managerOrder_) {
        result.push_back(t.get());
    }
    return result;
}

IComponentManager* Ecs::GetComponentManager(const Uid& uid) const
{
    if (auto pos = managers_.find(uid); pos != managers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Entity Ecs::CloneEntity(const Entity entity)
{
    if (!EntityUtil::IsValid(entity)) {
        return Entity();
    }

    const Entity clonedEntity = entityManager_.Create();
    if (entityManager_.IsAlive(entity)) {
        for (auto& cm : managerOrder_) {
            const auto id = cm->GetComponentId(entity);
            if (id != IComponentManager::INVALID_COMPONENT_ID) {
                cm->Create(clonedEntity);
                cm->SetData(clonedEntity, *cm->GetData(id));
            }
        }
    }
    return clonedEntity;
}

void Ecs::ProcessAdded()
{
    // Process added components.
    for (auto& m : managerOrder_) {
        vector<Entity> addedComponents = m->GetAddedComponents();
        if (!addedComponents.empty()) {
            // Let listeners know that there are new components.
            // global listeners
            for (auto* listener : componentListeners_) {
                if (listener) {
                    listener->OnComponentEvent(ComponentListener::EventType::CREATED, *m, addedComponents);
                }
            }
            // per manager listeners
            if (auto it = componentManagerListeners_.find(m.get()); it != componentManagerListeners_.end()) {
                for (auto* listener : it->second) {
                    if (listener) {
                        listener->OnComponentEvent(ComponentListener::EventType::CREATED, *m, addedComponents);
                    }
                }
            }
        }

        m->Gc();
    }
}

void Ecs::ProcessModified()
{
    for (auto& m : managerOrder_) {
        vector<Entity> updatedComponents = m->GetUpdatedComponents();
        if (!updatedComponents.empty()) {
            // Let listeners know that there are modified components.
            // global listeners
            for (auto* listener : componentListeners_) {
                if (listener) {
                    listener->OnComponentEvent(ComponentListener::EventType::MODIFIED, *m, updatedComponents);
                }
            }
            // per manager listeners
            if (auto it = componentManagerListeners_.find(m.get()); it != componentManagerListeners_.end()) {
                for (auto* listener : it->second) {
                    if (listener) {
                        listener->OnComponentEvent(ComponentListener::EventType::MODIFIED, *m, updatedComponents);
                    }
                }
            }
        }
    }
}

void Ecs::ProcessRemoved()
{
    // Get list of removed entities.
    vector<Entity> removed = entityManager_.GetRemovedEntities();

    // Process removed components (and remove components for removed entities also)
    for (auto& m : managerOrder_) {
        if (!removed.empty()) {
            // Destroy all components related to these entities.
            m->Destroy(removed);
        }

        vector<Entity> removedComponents = m->GetRemovedComponents();
        if (!removedComponents.empty()) {
            // Let listeners know that there are removed components.
            // global listeners
            for (auto* listener : componentListeners_) {
                if (listener) {
                    listener->OnComponentEvent(ComponentListener::EventType::DESTROYED, *m, removedComponents);
                }
            }
            // per manager listeners
            if (auto it = componentManagerListeners_.find(m.get()); it != componentManagerListeners_.end()) {
                for (auto* listener : it->second) {
                    if (listener) {
                        listener->OnComponentEvent(ComponentListener::EventType::DESTROYED, *m, removedComponents);
                    }
                }
            }
        }

        m->Gc();
    }
}

void Ecs::ProcessEvents()
{
    entityManager_.UpdateDeadEntities();

    // handle state changes (collect to groups of same kind of events)
    auto states = entityManager_.GetEvents();
    if (!states.empty()) {
        vector<Entity> res;
        auto type = states.front().second;
        for (const auto& s : states) {
            if (s.second != type) {
                if (!res.empty()) {
                    // Let listeners know that entity state has changed.
                    for (auto* listener : entityListeners_) {
                        if (listener) {
                            listener->OnEntityEvent(type, res);
                        }
                    }
                    // start collecting new events.
                    res.clear();
                }
                type = s.second;
            }
            // add to event list.
            res.push_back(s.first);
        }
        if (!res.empty()) {
            // Send the final events.
            for (auto* listener : entityListeners_) {
                if (listener) {
                    listener->OnEntityEvent(type, res);
                }
            }
        }
    }

    ProcessAdded();
    ProcessModified();
    ProcessRemoved();

    // Clean-up removed listeners.
    for (auto it = entityListeners_.begin(); it != entityListeners_.end();) {
        if (*it == nullptr) {
            it = entityListeners_.erase(it);
        } else {
            it++;
        }
    }
    for (auto it = componentListeners_.begin(); it != componentListeners_.end();) {
        if (*it == nullptr) {
            it = componentListeners_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = componentManagerListeners_.begin(); it != componentManagerListeners_.end();) {
        auto& listeners = it->second;
        for (auto listener = listeners.begin(); listener != listeners.end();) {
            if (*listener == nullptr) {
                listener = listeners.erase(listener);
            } else {
                ++listener;
            }
        }
        if (listeners.empty()) {
            it = componentManagerListeners_.erase(it);
        } else {
            ++it;
        }
    }
}

void Ecs::Initialize()
{
    for (auto& s : systemOrder_) {
        s->Initialize();
    }
}

bool Ecs::Update(uint64_t time, uint64_t delta)
{
    CORE_CPU_PERF_SCOPE("ECS", "Update", "Total_Cpu");

    bool frameRenderingQueued = false;
    if (GetRenderMode() == RENDER_ALWAYS || renderRequested_) {
        frameRenderingQueued = true;
    }

    // Update all systems.
    delta = static_cast<uint64_t>(delta * timeScale_);
    for (auto& s : systemOrder_) {
        CORE_CPU_PERF_SCOPE("ECS", "SystemUpdate_Cpu", s->GetName());
        if (s->Update(frameRenderingQueued, time, delta)) {
            frameRenderingQueued = true;
        }
    }

    // Clear modification flags from component managers.
    for (auto& componentManager : managerOrder_) {
        componentManager->ClearModifiedFlags();
    }

    renderRequested_ = false;
    needRender_ = frameRenderingQueued;

    return frameRenderingQueued;
}

void Ecs::Uninitialize()
{
    // Destroy all entities from scene.
    entityManager_.DestroyAllEntities();

    // Garbage-collect.
    ProcessEvents();

    // Uninitialize systems.
    for (auto it = systemOrder_.rbegin(); it != systemOrder_.rend(); ++it) {
        (*it)->Uninitialize();
    }
}

void Ecs::RequestRender()
{
    renderRequested_ = true;
}

void Ecs::SetRenderMode(RenderMode renderMode)
{
    renderMode_ = renderMode;
}

IEcs::RenderMode Ecs::GetRenderMode()
{
    return renderMode_;
}

bool Ecs::NeedRender() const
{
    return needRender_;
}

const IThreadPool::Ptr& Ecs::GetThreadPool() const
{
    return threadPool_;
}

float Ecs::GetTimeScale() const
{
    return timeScale_;
}

void Ecs::SetTimeScale(float scale)
{
    timeScale_ = scale;
}

void Ecs::Ref() noexcept
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void Ecs::Unref() noexcept
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

template<typename Container>
inline auto RemoveUid(Container& container, const Uid& uid)
{
    container.erase(std::remove_if(container.begin(), container.end(),
                        [&uid](const auto& thing) { return thing->GetUid() == uid; }),
        container.end());
}

void Ecs::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    if (type == EventType::ADDED) {
        // not really interesed in these events. systems and component managers are added when SystemGraphLoader parses
        // a configuration. we could store them in systems_ and managers_ and only define the order based on the graph.
    } else if (type == EventType::REMOVED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == SystemTypeInfo::UID) {
                const auto systemInfo = static_cast<const SystemTypeInfo*>(info);
                // for systems Untinitialize should be called before destroying the instance
                if (auto pos = systems_.find(systemInfo->uid); pos != systems_.end()) {
                    pos->second->Uninitialize();
                    systems_.erase(pos);
                }
                RemoveUid(systemOrder_, systemInfo->uid);
            } else if (info && info->typeUid == ComponentManagerTypeInfo::UID) {
                const auto managerInfo = static_cast<const ComponentManagerTypeInfo*>(info);
                // BaseManager expects that the component list is empty when it's destroyed. might be also
                // nice to notify all the listeners that the components are being destroyed.
                if (auto pos = managers_.find(managerInfo->uid); pos != managers_.end()) {
                    auto manager = pos->second;

                    // remove all the components.
                    const auto components = static_cast<IComponentManager::ComponentId>(manager->GetComponentCount());
                    for (IComponentManager::ComponentId i = 0; i < components; ++i) {
                        manager->Destroy(manager->GetEntity(i));
                    }

                    // check are there generic or specific component listeners to inform.
                    if (const auto listenerIt = componentManagerListeners_.find(manager);
                        !componentListeners_.empty() ||
                        ((listenerIt != componentManagerListeners_.end()) && !listenerIt->second.empty())) {
                        if (vector<Entity> removed = manager->GetRemovedComponents(); !removed.empty()) {
                            const auto removedView = array_view<Entity>(removed);
                            for (auto* lister : componentListeners_) {
                                if (lister) {
                                    lister->OnComponentEvent(
                                        ComponentListener::EventType::DESTROYED, *manager, removedView);
                                }
                            }
                            if (listenerIt != componentManagerListeners_.end()) {
                                for (auto* lister : listenerIt->second) {
                                    if (lister) {
                                        lister->OnComponentEvent(
                                            ComponentListener::EventType::DESTROYED, *manager, removedView);
                                    }
                                }
                                // remove all the listeners for this manager. RemoveListener won't do anything. this
                                // isn't neccessary, but rather not leave invalid manager pointer even if it's just used
                                // as the key.
                                componentManagerListeners_.erase(listenerIt);
                            }
                        }
                    }
                    // garbage collection will remove dead entries from the list and BaseManager is happy.
                    manager->Gc();
                    managers_.erase(pos);
                }
                RemoveUid(managerOrder_, managerInfo->uid);
            }
        }
    }
}
} // namespace

IEcs* IEcsInstance(IClassFactory& registry, const IThreadPool::Ptr& threadPool)
{
    return new Ecs(registry, threadPool);
}
CORE_END_NAMESPACE()
