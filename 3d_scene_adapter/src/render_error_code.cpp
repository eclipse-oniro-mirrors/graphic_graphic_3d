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

#include "scene_adapter/render_error_code.h"

namespace OHOS::Render3D {

const char* ToString(RenderErrorCode code)
{
    switch (code) {
        case RenderErrorCode::OK:                       return "OK";
        case RenderErrorCode::PARAM_RANGE_EXCEEDED:     return "Parameter exceeds the allowed range.";
        case RenderErrorCode::PARAM_MISMATCH:           return "Input parameter type or count mismatch.";
        case RenderErrorCode::SCENE_DESTROYED:          return "Scene used after destruction.";
        case RenderErrorCode::MEMORY_ALLOC_FAILED:      return "Memory allocation failed.";
        case RenderErrorCode::RESOURCE_DESTROYED:       return "Expired or Invalid resource.";
        default:                                        return "Undefined error code";
    }
}

} // namespace OHOS::Render3D