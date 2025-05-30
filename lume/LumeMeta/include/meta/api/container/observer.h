/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_API_CONTAINER_OBSERVER_H
#define META_API_CONTAINER_OBSERVER_H

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/interface/intf_container_observer.h>
#include <meta/interface/intf_object_hierarchy_observer.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper class for objects which implement IObjectHierarchyObserver.
 */
class ObjectHierarchyObserver : public InterfaceObject<IObjectHierarchyObserver> {
public:
    META_INTERFACE_OBJECT(ObjectHierarchyObserver, InterfaceObject<IObjectHierarchyObserver>, IObjectHierarchyObserver)
    META_INTERFACE_OBJECT_INSTANTIATE(ObjectHierarchyObserver, ClassId::ObjectHierarchyObserver)
    void SetTarget(const IObject::Ptr& root, HierarchyChangeModeValue mode)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SetTarget(root, mode));
    }
    void SetTarget(const IObject::Ptr& root)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SetTarget(root));
    }
    auto& Target(const IObject::Ptr& root)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SetTarget(root));
        return *this;
    }
    auto GetTarget() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetTarget());
    }
    template<typename Type = IObject>
    auto GetAllObserved() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(template GetAllObserved<Type>());
    }
    auto OnHierarchyChanged() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(OnHierarchyChanged());
    }
    /**
     * @brief Subscribe to IObjectHierarchyObserver::OnHierarchyChanged
     * @param callback Handler function for the event.
     * @return Event token for the handler which can be used to unsubscribe the handler. ContainerObserver does not
     * automatically unsubscribe the event at destruction.
     */
    template<typename Callback>
    auto OnHierarchyChanged(Callback&& callback) const
    {
        auto p = OnHierarchyChanged();
        return p ? p->AddHandler(MakeCallback<IOnHierarchyChanged>(BASE_NS::forward<Callback>(callback)))
                 : IEvent::Token {};
    }
};

/**
 * @brief Wrapper class for objects which implement IContainerObserver.
 */
class ContainerObserver : public InterfaceObject<IContainerObserver> {
public:
    META_INTERFACE_OBJECT(ContainerObserver, InterfaceObject<IContainerObserver>, IContainerObserver)
    META_INTERFACE_OBJECT_INSTANTIATE(ContainerObserver, ClassId::ContainerObserver)
    void SetContainer(const IContainer::Ptr& container)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SetContainer(container));
    }
    auto& Container(const IContainer::Ptr& container)
    {
        META_INTERFACE_OBJECT_CALL_PTR(SetContainer(container));
        return *this;
    }
    auto OnDescendantChanged() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(OnDescendantChanged());
    }
    /**
     * @brief Subscribe to IContainerObserver::OnDescendantChanged
     * @param callback Handler function for the event.
     * @return Event token for the handler which can be used to unsubscribe the handler. ContainerObserver does not
     * automatically unsubscribe the event at destruction.
     */
    template<typename Callback>
    auto OnDescendantChanged(Callback&& callback) const
    {
        auto p = OnDescendantChanged();
        return p ? p->AddHandler(MakeCallback<IOnChildChanged>(BASE_NS::forward<Callback>(callback)))
                 : META_NS::IEvent::Token {};
    }
};

/// Returns a default object which implements IObjectHierarchyObserver
template<>
inline auto CreateObjectInstance<IObjectHierarchyObserver>()
{
    return ObjectHierarchyObserver(CreateNew);
}
/// Returns a default object which implements IObjectHierarchyObserver
template<>
inline auto CreateObjectInstance<IContainerObserver>()
{
    return ContainerObserver(CreateNew);
}

META_END_NAMESPACE()

#endif // META_API_CONTAINER_OBSERVER_H
