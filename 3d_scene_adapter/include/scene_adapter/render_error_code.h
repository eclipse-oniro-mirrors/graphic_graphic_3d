/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_RENDER_3D_RENDER_ERROR_CODE_H
#define OHOS_RENDER_3D_RENDER_ERROR_CODE_H

#include <cstdint>

namespace OHOS::Render3D {

// Render error codes shared between scene_adapter (producer) and kits (consumer).
enum class RenderErrorCode : int32_t {
    OK                       = 0,
    INVALID_ARRAY_ELEMENT    = 25603001,  // Elements within a single array violate rules
    INVALID_PARAM            = 25603002,  // Function input parameters do not meet constraints
    ENGINE_DESTROYED         = 25601002,  // Scene has been destroyed, api called after destruction
    MEMORY_ALLOC_FAILED      = 25601004,  // Memory allocation failed
    RESOURCE_DESTROYED       = 25601006,  // Resource has been destroyed, api called after destruction
};

const char* ToString(RenderErrorCode code);

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_RENDER_ERROR_CODE_H