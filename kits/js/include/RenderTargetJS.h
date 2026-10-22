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

#ifndef RENDER_TARGET_JS_H
#define RENDER_TARGET_JS_H

#include "BaseObjectJS.h"
#include "SceneResourceImpl.h"
#include "RenderContextJS.h"
#include "export.h"

namespace OHOS::Render3D {
struct OffscreenCaptureData;
}

class SCENE_ADDON_PUBLIC RenderTargetJS final : public BaseObject, public SceneResourceImpl {
public:
    static constexpr uint32_t ID = 120;
    static void Init(napi_env env, napi_value exports);

    RenderTargetJS(napi_env, napi_callback_info);
    ~RenderTargetJS() override;
    void* GetInstanceImpl(uint32_t) override;

    void SetCaptureData(BASE_NS::shared_ptr<OHOS::Render3D::OffscreenCaptureData> data);

    SCENE_NS::IRenderTarget::Ptr GetEngineRenderTarget() const;

    static napi_value NewInstance(napi_env env);

    static RenderTargetJS* GetImpl(napi_env env, napi_value value);

private:
    void DisposeNative() override;
    void Finalize(napi_env env) override;

    napi_value GetWidth(NapiApi::FunctionContext<>& ctx);
    napi_value GetHeight(NapiApi::FunctionContext<>& ctx);

    napi_value GetPixelMap(NapiApi::FunctionContext<>& ctx);

    BASE_NS::shared_ptr<OHOS::Render3D::OffscreenCaptureData> captureData_;
    BASE_NS::shared_ptr<RenderResources> resources_;
    napi_ref pixelMapRef_ = nullptr;
};

#endif