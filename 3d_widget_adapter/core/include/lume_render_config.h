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

#ifndef OHOS_RENDER_3D_LUME_RENDER_CONFIG_H
#define OHOS_RENDER_3D_LUME_RENDER_CONFIG_H
#include <string>

#include <base/math/vector.h>

namespace OHOS::Render3D {

class LumeRenderConfig {
public:
    static LumeRenderConfig& GetInstance();
//     // lume common
    std::string renderBackend_;
    std::string systemGraph_;

private:
    LumeRenderConfig(const LumeRenderConfig&) = delete;
    LumeRenderConfig& operator=(const LumeRenderConfig&) = delete;
    LumeRenderConfig() = default;
    virtual ~LumeRenderConfig() = default;

    bool initialized_ = false;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_RENDER_CONFIG_H
