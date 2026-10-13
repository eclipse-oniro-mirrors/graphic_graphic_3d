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

#ifndef OHOS_RENDER_3D_FRAME_HOOK_REGISTRY_H
#define OHOS_RENDER_3D_FRAME_HOOK_REGISTRY_H

#include <memory>
#include <mutex>
#include <vector>

#include "render/intf_render_context.h"
#include "scene_adapter/intf_frame_hook.h"

namespace OHOS::Render3D {

// Concrete IFrameHookRegistry, a process-global Meyer's singleton (one
// render context per process). Hooks are stored as std::weak_ptr
// observations: a hook destroyed without Unregister() expires and is
// pruned by the render loop instead of dangling.
// Hooks are invoked while holding the mutex. This is safe because every
// destruction path Unregisters() first (parks on the mutex until the
// render pass finishes); a std strong reference could NOT keep a hook
// alive since META owns the lifetime (no-op deleter bridge in
// SurfaceStream). Hence hooks must not re-enter the registry from
// PreRender()/PostRender() (non-recursive mutex => deadlock).
class FrameHookRegistry : public IFrameHookRegistry {
public:
    static FrameHookRegistry& Instance()
    {
        static FrameHookRegistry instance;
        return instance;
    }

    void Register(const std::weak_ptr<IFrameHook>& hook) override;
    void Unregister(const std::weak_ptr<IFrameHook>& hook) override;

    void PreRenderAll(RENDER_NS::IRenderContext& ctx) override;
    void PostRenderAll(RENDER_NS::IRenderContext& ctx) override;

private:
    FrameHookRegistry() = default;
    ~FrameHookRegistry() override = default;

    FrameHookRegistry(const FrameHookRegistry&) = delete;
    FrameHookRegistry& operator=(const FrameHookRegistry&) = delete;
    FrameHookRegistry(const FrameHookRegistry&&) = delete;
    FrameHookRegistry& operator=(const FrameHookRegistry&&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<IFrameHook>> hooks_;
};

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_FRAME_HOOK_REGISTRY_H
