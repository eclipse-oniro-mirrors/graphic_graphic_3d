/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "3d/ecs/components/animation_state_component.h"
#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using CORE3D_NS::AnimationComponent;
using CORE3D_NS::AnimationStateComponent;
DECLARE_PROPERTY_TYPE(AnimationStateComponent::TrackState);
DECLARE_PROPERTY_TYPE(BASE_NS::vector<AnimationStateComponent::TrackState>);

DECLARE_PROPERTY_TYPE(AnimationComponent::PlaybackState);

ENUM_TYPE_METADATA(
    AnimationComponent::PlaybackState, ENUM_VALUE(STOP, "Stop"), ENUM_VALUE(PLAY, "Play"), ENUM_VALUE(PAUSE, "Pause"))

ENUM_TYPE_METADATA(AnimationStateComponent::FlagBits, ENUM_VALUE(MANUAL_PROGRESS, "Update Playback Time Manually."))
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class AnimationStateComponentManager final
    : public BaseManager<AnimationStateComponent, IAnimationStateComponentManager> {
    BEGIN_PROPERTY(AnimationStateComponent, componentMetaData_)
#include "3d/ecs/components/animation_state_component.h"
    END_PROPERTY();

public:
    explicit AnimationStateComponentManager(IEcs& ecs)
        : BaseManager<AnimationStateComponent, IAnimationStateComponentManager>(
              ecs, CORE_NS::GetName<AnimationStateComponent>())
    {}

    ~AnimationStateComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IAnimationStateComponentManagerInstance(IEcs& ecs)
{
    return new AnimationStateComponentManager(ecs);
}

void IAnimationStateComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<AnimationStateComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
