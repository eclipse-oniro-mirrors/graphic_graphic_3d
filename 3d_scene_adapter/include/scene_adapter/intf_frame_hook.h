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

#ifndef OHOS_RENDER_3D_INTF_FRAME_HOOK_H
#define OHOS_RENDER_3D_INTF_FRAME_HOOK_H

#include <memory>

#include <render/intf_render_context.h>

namespace OHOS::Render3D {

/// Hook invoked around the per-frame render submit (PreRender before
/// RenderDeferredFrame(), PostRender after). Bridges SceneAdapter (owns the
/// render loop) with SurfaceStream (a META attachment on Bitmap, otherwise
/// unreachable from the loop). Observed via std::weak_ptr: implementations
/// must keep the std control block alive for their whole lifetime (see
/// SurfaceStream) so weak_from_this() stays valid.
class IFrameHook : public std::enable_shared_from_this<IFrameHook> {
public:
    virtual ~IFrameHook() = default;

    /// Before the GPU submit: register GPU signals (FrameSignalScope::Register(ctx)).
    virtual void PreRender(RENDER_NS::IRenderContext& ctx) = 0;

    /// After the GPU submit: export signal fds (FrameSignalScope::TryExportFd(ctx))
    /// and feed them to buffer owners (SurfaceBufferOwner::SetReleaseFenceFd()).
    virtual void PostRender(RENDER_NS::IRenderContext& ctx) = 0;
};

/// Registry of frame hooks. SurfaceStream registers itself so the render
/// loop (SceneAdapter) can reach it despite the META attachment indirection.
/// Hooks are observed, never owned: a hook destroyed without Unregister()
/// expires and is pruned by the render loop.
class IFrameHookRegistry {
public:
    virtual ~IFrameHookRegistry() = default;

    /// At most once per hook (matched by control block); duplicates are
    /// rejected. Registering again after Unregister() is valid.
    virtual void Register(const std::weak_ptr<IFrameHook>& hook) = 0;

    /// Removes every registration of the hook; safe to call multiple times.
    virtual void Unregister(const std::weak_ptr<IFrameHook>& hook) = 0;

    virtual void PreRenderAll(RENDER_NS::IRenderContext& ctx) = 0;
    virtual void PostRenderAll(RENDER_NS::IRenderContext& ctx) = 0;
};

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_INTF_FRAME_HOOK_H
