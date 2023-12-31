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

#if !defined(ANIMATION_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define ANIMATION_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Animation component is the playback state for a collection of animations tracks.
 */
BEGIN_COMPONENT(IAnimationComponentManager, AnimationComponent)
#if !defined(IMPLEMENT_MANAGER)
    /** Playback states for an animation. */
    enum class PlaybackState : uint8_t { STOP = 0, PLAY = 1, PAUSE = 2 };
    /**  Special value for always repeating an animation. */
    static constexpr uint32_t REPEAT_COUNT_INFINITE = ~0u;
#endif
    /** State of the animation. */
    DEFINE_PROPERTY(PlaybackState, state, "Playback state", 0, VALUE(PlaybackState::STOP))
    /** Repeat the animation. */
    DEFINE_PROPERTY(uint32_t, repeatCount, "How many times the animation is played", 0, VALUE(1))
    /** Track start offset in seconds. Allows skipping the beginning of the animation tracks. */
    DEFINE_PROPERTY(float, startOffset, "Playback start offset.", 0, VALUE(0.0f))
    /** Track duration in seconds. Allows skipping the end of the animation tracks. */
    DEFINE_PROPERTY(float, duration, "", 0, VALUE(0.0f))
    /** Weight factor for this animation. */
    DEFINE_PROPERTY(float, weight, "Playback weight.", 0, VALUE(1.f))
    /** Playback speed. Negative speed plays the animation in reverse. */
    DEFINE_PROPERTY(float, speed, "Playback speed.", 0, VALUE(1.f))
    /** Tracks for this animation. */
    DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::EntityReference>, tracks, "Entity IDs for the tracks", 0,)

END_COMPONENT(IAnimationComponentManager, AnimationComponent, "65f8a318-8931-4bc8-8476-02863937b1fa")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // __ANIMATION_COMPONENT__
