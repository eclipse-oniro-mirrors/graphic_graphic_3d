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

#include "RenderTargetJS.h"

#include "JsObjectCache.h"
#include "ParamParsing.h"
#include "Promise.h"
#include "napi_error_util.h"
#include "pixel_map_napi.h"

#include <scene/interface/intf_render_target.h>
#include <scene/interface/intf_scene.h>

#include <scene_adapter/scene_adapter.h>

RenderTargetJS::RenderTargetJS(napi_env env, napi_callback_info info)
    : BaseObject(env, info), SceneResourceImpl(SceneResourceImpl::IMAGE)
{
    NapiApi::Scope scope(env);
}

RenderTargetJS::~RenderTargetJS() = default;

void RenderTargetJS::DisposeNative()
{
    captureData_.reset();
    pixelMapRef_ = nullptr;
}

void RenderTargetJS::Finalize(napi_env env)
{
    captureData_.reset();
}

void* RenderTargetJS::GetInstanceImpl(uint32_t id)
{
    if (id == RenderTargetJS::ID) {
        return this;
    }
    if (auto impl = SceneResourceImpl::GetInstanceImpl(id)) {
        return impl;
    }
    return BaseObject::GetInstanceImpl(id);
}

void RenderTargetJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> props;
    SceneResourceImpl::GetPropertyDescs(props);

    props.push_back(GetProperty<uint32_t, RenderTargetJS, &RenderTargetJS::GetWidth>("width"));
    props.push_back(GetProperty<uint32_t, RenderTargetJS, &RenderTargetJS::GetHeight>("height"));
    props.push_back(Method<FunctionContext<>, RenderTargetJS, &RenderTargetJS::GetPixelMap>("getPixelMap"));

    napi_value func;
    auto status = napi_define_class(env,
        "RenderTarget",
        NAPI_AUTO_LENGTH,
        BaseObject::ctor<RenderTargetJS>(),
        nullptr,
        props.size(),
        props.data(),
        &func);

    napi_set_named_property(env, exports, "RenderTarget", func);

    MyInstanceState* mis;
    MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (mis) {
        mis->StoreCtor("RenderTarget", func);
    }
}

napi_value RenderTargetJS::GetWidth(NapiApi::FunctionContext<>& ctx)
{
    if (captureData_) {
        return ctx.GetNumber(captureData_->width);
    }
    return ctx.GetNumber(static_cast<uint32_t>(0));
}

napi_value RenderTargetJS::GetHeight(NapiApi::FunctionContext<>& ctx)
{
    if (captureData_) {
        return ctx.GetNumber(captureData_->height);
    }
    return ctx.GetNumber(static_cast<uint32_t>(0));
}

napi_value RenderTargetJS::GetPixelMap(NapiApi::FunctionContext<>& ctx)
{
    auto env = ctx.GetEnv();
    auto promise = Promise(env);

    if (!captureData_) {
        promise.Reject(MakeErrorObject(env, NapiErrorUtil::RESOURCE_DESTROYED, nullptr));
        return promise;
    }

    if (!captureData_->pixelMap) {
        promise.Reject("getPixelMap failed: pixelMap is null");
        return promise;
    }

    captureData_->pixelMapRequested = true;
    captureData_->pixelMap->SetEditable(false);

    napi_value pixelMapJs = OHOS::Media::PixelMapNapi::CreatePixelMap(env, captureData_->pixelMap);
    if (pixelMapJs == nullptr) {
        promise.Reject("getPixelMap failed: could not create PixelMap JS Object");
        return promise;
    }

    if (pixelMapRef_) {
        napi_delete_reference(env, pixelMapRef_);
    }
    napi_create_reference(env, pixelMapJs, 1, &pixelMapRef_);

    promise.Resolve(pixelMapJs);
    return promise;
}

void RenderTargetJS::SetCaptureData(BASE_NS::shared_ptr<OHOS::Render3D::OffscreenCaptureData> data)
{
    captureData_ = std::move(data);
    if (captureData_ && captureData_->engineBitmap) {
        SetNativeObject(interface_pointer_cast<META_NS::IObject>(captureData_->engineBitmap), PtrType::WEAK);
    }
}

SCENE_NS::IRenderTarget::Ptr RenderTargetJS::GetEngineRenderTarget() const
{
    if (captureData_) {
        return captureData_->engineBitmap;
    }
    return {};
}

napi_value RenderTargetJS::NewInstance(napi_env env)
{
    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, reinterpret_cast<void**>(&mis));
    if (!mis) {
        return nullptr;
    }
    auto ctor = mis->FetchCtor("RenderTarget");
    if (!ctor) {
        return nullptr;
    }
    napi_value instance;
    napi_new_instance(env, ctor, 0, nullptr, &instance);
    return instance;
}

RenderTargetJS* RenderTargetJS::GetImpl(napi_env env, napi_value value)
{
    if (!value) {
        return nullptr;
    }
    NapiApi::Object obj(env, value);
    if (auto* root = obj.GetRoot()) {
        return static_cast<RenderTargetJS*>(root->GetInstanceImpl(RenderTargetJS::ID));
    }
    return nullptr;
}