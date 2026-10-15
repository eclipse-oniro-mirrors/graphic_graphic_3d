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

#include "scene_adapter/surface_stream.h"

#include <vector>

#include "3d/ecs/components/environment_component.h"
#include "3d/ecs/components/render_handle_component.h"

#include "core/image/intf_image_loader_manager.h"
#include "core/intf_engine.h"
#include "core/plugin/intf_class_register.h"

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"
#include "GLES3/gl31.h"

#include "ibuffer_consumer_listener.h"
#include "iconsumer_surface.h"

#include "render/device/intf_gpu_resource_manager.h"
#include "render/device/intf_shader_manager.h"
#if RENDER_HAS_GLES_BACKEND || RENDER_HAS_GL_BACKEND
#include "render/gles/intf_device_gles.h"
#endif
#include "render/implementation_uids.h"
#include "render/intf_render_context.h"
#if RENDER_HAS_VULKAN_BACKEND
#include "render/vulkan/intf_device_vk.h"
#endif

#include "scene/ext/intf_ecs_context.h"
#include "scene/ext/intf_ecs_object_access.h"
#include "scene/ext/intf_ecs_object.h"
#include "scene/ext/intf_render_resource.h"
#include "scene/ext/util.h"
#include "scene/interface/intf_application_context.h"
#include "scene/interface/intf_image.h"
#include "scene_adapter/frame_hook_registry.h"
#include "surface_buffer.h"
#include "surface_utils.h"
#include "sync_fence.h"

namespace OHOS::Render3D {

namespace {
static constexpr uint32_t MAX_IMAGE_EXTENT = 32768U;
// Queue size 3 = 2 cached consumer buffers + 1 free slot for the producer.
static constexpr uint32_t SURFACE_QUEUE_SIZE = 3;

OH_NativeBuffer_ColorSpace ConvertColorGamutToColorSpace(OHOS::GraphicColorGamut colorGamut)
{
    switch (colorGamut) {
        case OHOS::GRAPHIC_COLOR_GAMUT_STANDARD_BT601:
            return OH_COLORSPACE_BT601_EBU_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_STANDARD_BT709:
            return OH_COLORSPACE_BT709_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_SRGB:
            return OH_COLORSPACE_SRGB_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_ADOBE_RGB:
            return OH_COLORSPACE_ADOBERGB_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_DISPLAY_P3:
            return OH_COLORSPACE_P3_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2100_PQ:
            return OH_COLORSPACE_BT2020_PQ_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2100_HLG:
            return OH_COLORSPACE_BT2020_HLG_FULL;
        case OHOS::GRAPHIC_COLOR_GAMUT_BT2020:
        case OHOS::GRAPHIC_COLOR_GAMUT_DISPLAY_BT2020:
            return OH_COLORSPACE_DISPLAY_BT2020_SRGB;
        default:
            return OH_COLORSPACE_SRGB_FULL;
    }
}
} // namespace

bool SurfaceStream::Build(const META_NS::IMetadata::Ptr& metadata)
{
    if (metadata == nullptr) {
        CORE_LOG_E("SurfaceStream Build metadata is nullptr");
        return false;
    }

    if (!Super::Build(metadata)) {
        CORE_LOG_E("SurfaceStream Build failed");
        return false;
    }

    auto context = SCENE_NS::GetInterfaceBuildArg<SCENE_NS::IRenderContext>(metadata, "RenderContext");
    if (context == nullptr) {
        auto app = SCENE_NS::GetDefaultApplicationContext();
        if (app == nullptr) {
            CORE_LOG_E("get default application context failed");
            return false;
        }
        context = app->GetRenderContext();
        if (context == nullptr) {
            CORE_LOG_E("get default render context failed");
            return false;
        }
    }

    engineQueue_ = context->GetRenderQueue();
    if (engineQueue_ == nullptr) {
        CORE_LOG_E("get render queue failed");
        return false;
    }

    renderContext_ = context->GetRenderer();
    if (renderContext_) {
        backendType_ = renderContext_->GetDevice().GetBackendType();
    }
    return true;
}

bool SurfaceStream::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    if (auto existing = renderResource_.lock()) {
        CORE_LOG_E("render resource already attached.");
        return false;
    }

    if (target == nullptr) {
        CORE_LOG_E("target is null");
        return false;
    }

    auto bitmap = interface_cast<SCENE_NS::IBitmap>(target);
    if (bitmap == nullptr) {
        CORE_LOG_E("Incorrect type bitmap is null");
        return false;
    }
    renderResource_ = interface_pointer_cast<SCENE_NS::IRenderResource>(target);
    if (auto attached = renderResource_.lock(); !attached) {
        CORE_LOG_E("render resource attach failed");
        return false;
    }

    if (!Init()) {
        CORE_LOG_E("surface stream Init failed");
        // Roll back the render resource binding on init failure to keep state consistent.
        renderResource_ = nullptr;
        return false;
    }

    return true;
}

bool SurfaceStream::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    if (target == nullptr) {
        CORE_LOG_E("detach target is null");
        return false;
    }
    if (interface_pointer_cast<IAttach>(renderResource_) != interface_pointer_cast<IAttach>(target)) {
        CORE_LOG_E("Invalid input parameter, incorrect type");
        return false;
    }
    renderResource_ = nullptr;

    Deinit();
    return true;
}

bool SurfaceStream::Init()
{
    auto consumerSurface = OHOS::IConsumerSurface::Create("LumeSurfaceConsumer");
    if (consumerSurface == nullptr) {
        CORE_LOG_E("create surface consumer failed");
        return false;
    }
    auto producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        CORE_LOG_E("create surface producer failed");
        return false;
    }
    producerSurface_ = OHOS::Surface::CreateSurfaceAsProducer(producer);
    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (producerSurface_ == nullptr || utils == nullptr) {
        CORE_LOG_E("get producer surface or utils failed");
        producerSurface_ = nullptr;
        return false;
    }
    producerSurface_->SetQueueSize(SURFACE_QUEUE_SIZE);
    surfaceId_.store(producerSurface_->GetUniqueId(), std::memory_order_relaxed);
    utils->Add(surfaceId_.load(std::memory_order_relaxed), producerSurface_);

    // SurfaceBufferOwner owns the consumer surface and buffer lifecycle.
    bufferOwner_ = std::make_unique<SurfaceBufferOwner>(consumerSurface);
    consumerSurface->RegisterConsumerListener(this);

    // Frame hook registration lets the render loop (SceneAdapter) invoke
    // PreRender/PostRender to produce release fences. The no-op-deleter
    // holder initializes the std control block for weak_from_this().
    if (!selfHookHolder_) {
        selfHookHolder_ = std::shared_ptr<IFrameHook>(static_cast<IFrameHook*>(this), [](IFrameHook*) {});
    }
    FrameHookRegistry::Instance().Register(weak_from_this());

    return true;
}

void SurfaceStream::Deinit()
{
    // Unregister first, then tear down.
    FrameHookRegistry::Instance().Unregister(weak_from_this());

    if (bufferOwner_) {
        auto surface = bufferOwner_->GetSurface();
        if (surface) {
            auto ret = surface->UnregisterConsumerListener();
            if (ret != GSERROR_OK) {
                CORE_LOG_E("unregister consumer listener failed");
            }
        }
    }

    auto utils = OHOS::SurfaceUtils::GetInstance();
    if (utils) {
        utils->Remove(surfaceId_.load(std::memory_order_relaxed));
    }
    surfaceId_.store(0, std::memory_order_relaxed);

    producerSurface_ = nullptr;
    // ~SurfaceBufferOwner() calls ReleaseAll().
    bufferOwner_.reset();
}

void SurfaceStream::OnBufferAvailable()
{
    if (engineQueue_ == nullptr) {
        CORE_LOG_E("engineQueue is nullptr");
        return;
    }
    META_NS::AddFutureTaskOrRunDirectly(engineQueue_, [this]() { ProcessBufferAvailable(); }).Wait();
}

void SurfaceStream::ProcessBufferAvailable()
{
    if (!bufferOwner_) {
        CORE_LOG_E("invalid buffer owner");
        return;
    }

    OHOS::sptr<OHOS::SurfaceBuffer> acquireSurfaceBuffer = nullptr;
    OHOS::sptr<OHOS::SyncFence> acquireFence = OHOS::SyncFence::INVALID_FENCE;
    int64_t timestamp = 0;
    OHOS::Rect damage = {0, 0, 0, 0};
    if (!bufferOwner_->Acquire(acquireSurfaceBuffer, acquireFence, timestamp, damage)) {
        return;
    }

    // Wait on the acquire fence (GPU-side preferred, CPU fallback).
    auto waitResult = WaitAcquireFence(renderContext_, acquireFence);
    // Drop the SyncFence: WaitAcquireFence fully manages the fence fd
    // (dups and closes it internally), so dropping it only closes the
    // original fd.
    acquireFence = OHOS::SyncFence::INVALID_FENCE;
    if (waitResult != AcquireFenceWaitResult::GPU_WAITED && waitResult != AcquireFenceWaitResult::CPU_WAITED) {
        CORE_LOG_E("wait acquire fence failed, release buffer");
        bufferOwner_->ReleaseImmediate(acquireSurfaceBuffer);
        return;
    }

    OH_NativeBuffer* nativeBuffer = acquireSurfaceBuffer->SurfaceBufferToNativeBuffer();
    if (nativeBuffer == nullptr) {
        CORE_LOG_E("surface buffer to native buffer failed");
        bufferOwner_->ReleaseImmediate(acquireSurfaceBuffer);
        return;
    }
    uint32_t width = static_cast<uint32_t>(acquireSurfaceBuffer->GetSurfaceBufferWidth());
    uint32_t height = static_cast<uint32_t>(acquireSurfaceBuffer->GetSurfaceBufferHeight());
    width_.store(width, std::memory_order_relaxed);
    height_.store(height, std::memory_order_relaxed);

    UpdateView(nativeBuffer, width, height, acquireSurfaceBuffer->GetSurfaceBufferColorGamut());

    // Bind one signal handle to this buffer for its whole cache lifetime: it
    // is re-registered every frame in PreRender and the release fence fd is
    // attached to the same entry in PostRender (1 buffer : 1 handle).
    RENDER_NS::RenderHandleReference signalHandle = FrameSignalHandleAllocator::Instance().Alloc();
    bufferOwner_->Cache(BASE_NS::make_shared<BufferCacheEntry>(acquireSurfaceBuffer, signalHandle));
}

void SurfaceStream::PreRender(RENDER_NS::IRenderContext& ctx)
{
    if (!bufferOwner_) {
        CORE_LOG_E("%s bufferOwner is nullptr", __func__);
        return;
    }

    // Re-register the handle bound to the latest cached buffer. The handle
    // was allocated once when the buffer arrived and stays valid across
    // frames: RenderFrameUtil::AddGpuSignal creates a fresh semaphore per
    // frame and matches entries by handle only within that frame.
    RENDER_NS::RenderHandleReference handle;
    if (!bufferOwner_->GetLatestSignalHandle(handle)) {
        // No cached buffer to sample; nothing to guard.
        CORE_LOG_E("%s get latest signal handle failed", __func__);
        return;
    }
    frameSignalScope_.emplace(handle);
    frameSignalScope_->Register(ctx);
}

void SurfaceStream::PostRender(RENDER_NS::IRenderContext& ctx)
{
    if (!frameSignalScope_.has_value()) {
        // PreRender did not register a signal (no cached buffer).
        CORE_LOG_E("%s frameSignalScope not register a signal", __func__);
        return;
    }
    // Export the release fence (the SyncFence owns its fd) and attach it to
    // the cache entry bound to this handle; consumed when the entry is
    // released. An idle frame yields an invalid fence and keeps any
    // previously stored fence.
    OHOS::sptr<OHOS::SyncFence> releaseFence = frameSignalScope_->TryExportFence(ctx);
    if (bufferOwner_) {
        bufferOwner_->SetReleaseFence(frameSignalScope_->GetHandle(), releaseFence);
    } else {
        // Teardown raced with this frame: no cache entry owns the exported
        // fence anymore. The local sptr is the last reference, so the fence
        // closes its own fd on destruction; nothing to do.
        CORE_LOG_E("%s bufferOwner is nullptr, fence dropped (fd auto-closed)", __func__);
    }
    frameSignalScope_.reset();
}

void SurfaceStream::UpdateView(
    OH_NativeBuffer* buffer, uint32_t width, uint32_t height, OHOS::GraphicColorGamut colorGamut)
{
    if (buffer == nullptr || width == 0 || height == 0 || width > MAX_IMAGE_EXTENT || height > MAX_IMAGE_EXTENT) {
        CORE_LOG_E("invalid native buffer or width(%u) or height(%u)", width, height);
        return;
    }

    if (renderContext_ == nullptr) {
        CORE_LOG_E("invalid render context");
        return;
    }

    auto colorSpace = ConvertColorGamutToColorSpace(colorGamut);
    int32_t colorSpaceResult = OH_NativeBuffer_SetColorSpace(buffer, colorSpace);
    if (colorSpaceResult != 0) {
        CORE_LOG_E("OH_NativeBuffer_SetColorSpace failed: %d", colorSpaceResult);
    }
    auto& device = renderContext_->GetDevice();
    auto& grm = device.GetGpuResourceManager();
    RENDER_NS::GpuImageDesc imageDesc{};
    imageDesc.usageFlags = RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    imageDesc.width = width;
    imageDesc.height = height;
    RENDER_NS::RenderHandleReference handle;
    BASE_NS::string frameId = "Internal://SurfaceStream_";
    frameId += BASE_NS::to_string(surfaceId_.load(std::memory_order_relaxed)) + "_" +
        BASE_NS::to_string(frameIndex_.fetch_add(1, std::memory_order_relaxed));

    if (backendType_ == RENDER_NS::DeviceBackendType::VULKAN) {
        RENDER_NS::ImageDescVk vkImageDesc{};
        vkImageDesc.platformHwBuffer = reinterpret_cast<uintptr_t>(buffer);
        vkImageDesc.isFormatEffectivelySet = true;
        handle = grm.CreateView(frameId, imageDesc, vkImageDesc);
    } else if (backendType_ == RENDER_NS::DeviceBackendType::OPENGLES) {
        RENDER_NS::ImageDescGLES glImageDesc{};
        glImageDesc.type = GL_TEXTURE_EXTERNAL_OES;
        glImageDesc.platformHwBuffer = reinterpret_cast<uintptr_t>(buffer);
        handle = grm.CreateView(frameId, imageDesc, glImageDesc);
    } else {
        CORE_LOG_E("unsupported backend type: %d", static_cast<int>(backendType_));
        return;
    }

    UpdateRenderResourceHandle(handle);
}

void SurfaceStream::UpdateRenderResourceHandle(const RENDER_NS::RenderHandleReference& handle)
{
    auto bitmap = renderResource_.lock();
    if (bitmap == nullptr) {
        CORE_LOG_E("bitmap is nullptr");
        return;
    }
    if (handle) {
        bitmap->SetRenderHandle(handle);
    }
}

SurfaceStream::~SurfaceStream()
{
    // Deinit() unregisters the hook/listener and releases all cached buffers.
    Deinit();

    renderResource_ = nullptr;
    renderContext_ = nullptr;
    engineQueue_ = nullptr;
}

} // namespace OHOS::Render3D