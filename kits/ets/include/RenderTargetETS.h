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

#ifndef OHOS_3D_RENDER_TARGET_ETS_H
#define OHOS_3D_RENDER_TARGET_ETS_H

#include <memory>

#include "ImageETS.h"

#include <scene/interface/intf_render_target.h>
#include "scene_adapter/scene_adapter.h"

namespace OHOS::Render3D {
class RenderTargetETS : public ImageETS {
public:
    explicit RenderTargetETS(BASE_NS::shared_ptr<OffscreenCaptureData> captureData);
    ~RenderTargetETS() override;
    void Destroy() override;

    META_NS::IObject::Ptr GetNativeObj() const override;
    SCENE_NS::IImage::Ptr GetNativeImage() const override;

    int32_t GetWidth() const;
    int32_t GetHeight() const;

    std::shared_ptr<OHOS::Media::PixelMap> GetPixelMap() const;

    SCENE_NS::IRenderTarget::Ptr GetEngineRenderTarget() const;

private:
    BASE_NS::shared_ptr<OffscreenCaptureData> captureData_;
};
} // namespace OHOS::Render3D
#endif  // OHOS_3D_RENDER_TARGET_ETS_H