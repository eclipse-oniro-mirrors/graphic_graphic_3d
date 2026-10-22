/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_RENDER_TARGET_IMPL_H
#define OHOS_3D_RENDER_TARGET_IMPL_H

#include "ImageImpl.h"
#include "RenderTargetETS.h"

namespace OHOS::Render3D::KITETS {
class RenderTargetImpl : public ImageImpl {
public:
    explicit RenderTargetImpl(const std::shared_ptr<RenderTargetETS> rtETS);
    ~RenderTargetImpl() override;
    void destroy() override;

    int32_t getWidth();
    int32_t getHeight();

    uintptr_t getPixelMapSync();

    std::shared_ptr<RenderTargetETS> getInternalRenderTarget() const
    {
        return rtETS_;
    }

private:
    std::shared_ptr<RenderTargetETS> rtETS_;
};
} // namespace OHOS::Render3D::KITETS
#endif  // OHOS_3D_RENDER_TARGET_IMPL_H