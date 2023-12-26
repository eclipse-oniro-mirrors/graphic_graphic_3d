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

#include <3d/ecs/components/layer_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class LayerComponentManager final : public BaseManager<LayerComponent, ILayerComponentManager> {
    BEGIN_PROPERTY(LayerComponent, ComponentMetadata)
#include <3d/ecs/components/layer_component.h>
    END_PROPERTY();
    const array_view<const Property> ComponentMetaData { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit LayerComponentManager(IEcs& ecs)
        : BaseManager<LayerComponent, ILayerComponentManager>(ecs, CORE_NS::GetName<LayerComponent>())
    {}
    ~LayerComponentManager() = default;
    size_t PropertyCount() const override
    {
        return ComponentMetaData.size();
    }
    const Property* MetaData(size_t index) const override
    {
        if (index < ComponentMetaData.size()) {
            return &ComponentMetaData[index];
        }
        return nullptr;
    }
    array_view<const Property> MetaData() const override
    {
        return ComponentMetaData;
    }
};

IComponentManager* ILayerComponentManagerInstance(IEcs& ecs)
{
    return new LayerComponentManager(ecs);
}

void ILayerComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<LayerComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()