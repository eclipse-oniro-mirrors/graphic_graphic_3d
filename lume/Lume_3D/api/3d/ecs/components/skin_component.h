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

#if !defined(API_3D_ECS_COMPONENTS_SKIN_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_SKIN_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Skin component is used to store which skin/ skeleton is used for skinning the entity.
 */
BEGIN_COMPONENT(ISkinComponentManager, SkinComponent)

    /** Entity which defines the skin /skeleton structure and initial pose.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, skin, "Skin IBMs", 0, VALUE(~0u))

    /** Skin root entity that is used to calculate joint transformations.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, skinRoot, "Skin Root", 0, VALUE(~0u))

    /** Skeleton root (if present) points to the node that is the common root of the joint entity hierarchy.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, skeleton, "Skeleton Root", 0, VALUE(~0u))

END_COMPONENT(ISkinComponentManager, SkinComponent, "683dddf3-6670-4bd2-827b-8e8a5484f0cf")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
