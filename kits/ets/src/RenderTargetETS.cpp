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

#include "RenderTargetETS.h"

namespace OHOS::Render3D {
RenderTargetETS::RenderTargetETS(BASE_NS::shared_ptr<OffscreenCaptureData> captureData)
    : ImageETS("", "", nullptr), captureData_(std::move(captureData))
{
    if (captureData_ && captureData_->engineBitmap) {
        auto named = interface_cast<META_NS::INamed>(captureData_->engineBitmap);
        if (named) {
            auto name = META_NS::GetValue(named->Name());
            SetName(std::string(name.data(), name.size()));
        }
    }
}

RenderTargetETS::~RenderTargetETS()
{
    Destroy();
}

void RenderTargetETS::Destroy()
{
    if (captureData_) {
        captureData_.reset();
    }
}

META_NS::IObject::Ptr RenderTargetETS::GetNativeObj() const
{
    if (captureData_ && captureData_->engineBitmap) {
        return interface_pointer_cast<META_NS::IObject>(captureData_->engineBitmap);
    }
    return {};
}

SCENE_NS::IImage::Ptr RenderTargetETS::GetNativeImage() const
{
    if (captureData_ && captureData_->engineBitmap) {
        return interface_pointer_cast<SCENE_NS::IImage>(captureData_->engineBitmap);
    }
    return {};
}

int32_t RenderTargetETS::GetWidth() const
{
    return captureData_ ? static_cast<int32_t>(captureData_->width) : 0;
}

int32_t RenderTargetETS::GetHeight() const
{
    return captureData_ ? static_cast<int32_t>(captureData_->height) : 0;
}

std::shared_ptr<OHOS::Media::PixelMap> RenderTargetETS::GetPixelMap() const
{
    if (captureData_) {
        captureData_->pixelMapRequested = true;
        return captureData_->pixelMap;
    }
    return nullptr;
}

SCENE_NS::IRenderTarget::Ptr RenderTargetETS::GetEngineRenderTarget() const
{
    if (captureData_) {
        return captureData_->engineBitmap;
    }
    return {};
}
}  // namespace OHOS::Render3D