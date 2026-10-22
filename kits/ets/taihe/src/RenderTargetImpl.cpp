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

#include "RenderTargetImpl.h"

#include <atomic>
#include <mutex>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include <ani_signature_builder.h>
#include "pixel_map_taihe_ani.h"
#include "TaiheErrorUtil.h"

namespace {
ani_ref gRtImageNamespace{};
ani_function gRtCreatePixelMapByPtr{};
std::atomic<bool> gRtCreatePixelMapByPtrInited{false};
std::mutex gRtCreatePixelMapByPtrMutex;

bool InitRtCreatePixelMapByPtr(ani_env* env)
{
    if (gRtCreatePixelMapByPtrInited.load(std::memory_order_acquire)) {
        return true;
    }
    std::lock_guard<std::mutex> lock(gRtCreatePixelMapByPtrMutex);
    if (gRtCreatePixelMapByPtrInited.load(std::memory_order_relaxed)) {
        return true;
    }
    arkts::ani_signature::Namespace ns = arkts::ani_signature::Builder::BuildNamespace("@ohos.multimedia.image.image");
    ani_namespace imageNamespace;
    if (env->FindNamespace(ns.Descriptor().c_str(), &imageNamespace) != ANI_OK) {
        return false;
    }
    if (env->GlobalReference_Create(static_cast<ani_ref>(imageNamespace), &gRtImageNamespace) != ANI_OK) {
        return false;
    }
    if (env->Namespace_FindFunction(imageNamespace, "createPixelMapByPtr", nullptr, &gRtCreatePixelMapByPtr) !=
        ANI_OK) {
        if (env->GlobalReference_Delete(gRtImageNamespace) != ANI_OK) {
            WIDGET_LOGE("InitRtCreatePixelMapByPtr: GlobalReference_Delete failed");
        }
        gRtImageNamespace = nullptr;
        return false;
    }
    gRtCreatePixelMapByPtrInited.store(true, std::memory_order_release);
    return true;
}
} // namespace

namespace OHOS::Render3D::KITETS {
RenderTargetImpl::RenderTargetImpl(const std::shared_ptr<RenderTargetETS> rtETS) : ImageImpl(rtETS), rtETS_(rtETS)
{}

RenderTargetImpl::~RenderTargetImpl()
{
    destroy();
}

void RenderTargetImpl::destroy()
{
    if (rtETS_) {
        rtETS_->Destroy();
        rtETS_.reset();
    }
}

int32_t RenderTargetImpl::getWidth()
{
    return rtETS_ ? rtETS_->GetWidth() : 0;
}

int32_t RenderTargetImpl::getHeight()
{
    return rtETS_ ? rtETS_->GetHeight() : 0;
}

uintptr_t RenderTargetImpl::getPixelMapSync()
{
    if (!rtETS_) {
        SetBusinessError(ErrorCode::RESOURCE_DESTROYED, nullptr);
        return 0;
    }
    auto pixelMap = rtETS_->GetPixelMap();
    if (!pixelMap) {
        return 0;
    }
    pixelMap->SetEditable(false);

    taihe::env_guard guard;
    ani_env* env = guard.get_env();
    if (!env) {
        return 0;
    }

    if (!InitRtCreatePixelMapByPtr(env)) {
        return 0;
    }

    std::unique_ptr<OHOS::Media::PixelMapTaiheAni> pixelMapAni = std::make_unique<OHOS::Media::PixelMapTaiheAni>();
    pixelMapAni->nativePixelMap_ = pixelMap;
    ani_ref pixelMapObj;
    if (env->Function_Call_Ref(gRtCreatePixelMapByPtr, &pixelMapObj, reinterpret_cast<ani_long>(pixelMapAni.get())) ==
        ANI_OK) {
        pixelMapAni.release();
        return reinterpret_cast<uintptr_t>(pixelMapObj);
    }
    return 0;
}
}  // namespace OHOS::Render3D::KITETS