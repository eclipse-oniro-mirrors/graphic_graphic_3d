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

#if !defined(MORPH_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define MORPH_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Component for giving weights to morph targets. */
BEGIN_COMPONENT(IMorphComponentManager, MorphComponent)
    /** Name of each morph target. */
    DEFINE_PROPERTY(BASE_NS::vector<BASE_NS::string>, morphNames, "", PropertyFlags::IS_HIDDEN,)
    /** Weight of each morph target. The */
    DEFINE_PROPERTY(BASE_NS::vector<float>, morphWeights, "@morphNames[*]", PropertyFlags::IS_SLIDER,)
END_COMPONENT(IMorphComponentManager, MorphComponent, "46f1ddb9-053d-4f54-aaea-ddfca075294f")

#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
