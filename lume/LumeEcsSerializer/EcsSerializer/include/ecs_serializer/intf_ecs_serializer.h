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

#ifndef API_ECS_SERIALIZER_ECSSERIALIZER_H
#define API_ECS_SERIALIZER_ECSSERIALIZER_H

#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/json/json.h>
#include <core/property/intf_property_api.h>
#include <core/property/property.h>
#include <ecs_serializer/intf_entity_collection.h>
#include <ecs_serializer/namespace.h>
#include <render/resource_handle.h>
#include <util/io_util.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

class IEcsSerializer {
public:
    static const uint32_t VERSION_MAJOR { 22u };
    static const uint32_t VERSION_MINOR { 0u };

    struct EntityInfo {
        Entity entity;
        string srcUri;
        string contextUri;
    };

    struct ExternalCollection {
        string src;
        string contextUri;
    };

    class IListener {
    public:
        virtual IEntityCollection* GetExternalCollection(
            IEcs& ecs, string_view uri, string_view contextUri) = 0;

    protected:
        virtual ~IListener() = default;
    };

    class IPropertySerializer {
    public:
        virtual bool ToJson(const IEntityCollection& ec, const Property& property, uintptr_t offset,
            json::standalone_value& jsonOut) const = 0;
        virtual bool FromJson(const IEntityCollection& ec, const json::value& jsonIn,
            const Property& property, uintptr_t offset) const = 0;

    protected:
        virtual ~IPropertySerializer() = default;
    };

    virtual void SetListener(IListener* listener) = 0;

    virtual void SetDefaultSerializers() = 0;
    virtual void SetSerializer(const PropertyTypeDecl& type, IPropertySerializer& serializer) = 0;

    virtual bool WriteEntityCollection(const IEntityCollection& ec, json::standalone_value& jsonOut) const = 0;
    virtual bool WriteComponents(
        const IEntityCollection& ec, Entity entity, json::standalone_value& jsonOut) const = 0;
    virtual bool WriteComponent(const IEntityCollection& ec, Entity entity,
        const IComponentManager& cm, IComponentManager::ComponentId id,
        json::standalone_value& jsonOut) const = 0;
    virtual bool WriteProperty(const IEntityCollection& ec, const Property& property, uintptr_t offset,
        json::standalone_value& jsonOut) const = 0;

    virtual bool GatherExternalCollections(const json::value& jsonIn, string_view contextUri,
        vector<ExternalCollection>& externalCollectionsOut) const = 0;

    virtual UTIL_NS::IoUtil::SerializationResult ReadEntityCollection(
        IEntityCollection& ec, const json::value& jsonIn, string_view contextUri) const = 0;
    virtual bool ReadComponents(IEntityCollection& ec, const json::value& jsonIn) const = 0;
    virtual bool ReadComponent(IEntityCollection& ec, const json::value& jsonIn, Entity entity,
        IComponentManager& component) const = 0;
    virtual bool ReadProperty(IEntityCollection& ec, const json::value& jsonIn,
        const Property& property, uintptr_t offset) const = 0;

    virtual RENDER_NS::RenderHandleReference LoadImageResource(string_view uri) const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IEcsSerializer* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = unique_ptr<IEcsSerializer, Deleter>;

protected:
    IEcsSerializer() = default;
    virtual ~IEcsSerializer() = default;
    virtual void Destroy() = 0;
};

ECS_SERIALIZER_END_NAMESPACE()

#endif // API_ECS_SERIALIZER_SCENESERIALIZER_H
