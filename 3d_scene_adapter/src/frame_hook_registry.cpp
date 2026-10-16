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

#include "scene_adapter/frame_hook_registry.h"

#include <algorithm>

#include "core/log.h"

namespace OHOS::Render3D {

void FrameHookRegistry::Register(const std::weak_ptr<IFrameHook>& hook)
{
    // expired() also covers an empty (never-set) weak_ptr.
    if (hook.expired()) {
        CORE_LOG_E("%s hook is nullptr or expired", __func__);
        return;
    }
    // A hook must be registered at most once; a duplicate would make the
    // render loop invoke its PreRender()/PostRender() twice per frame.
    // Match by control block (owner_before), consistent with Unregister(),
    // so this also detects an expired leftover entry of the same hook.
    const auto sameControlBlock = [&hook](const std::weak_ptr<IFrameHook>& entry) {
        return !hook.owner_before(entry) && !entry.owner_before(hook);
    };
    if (std::any_of(hooks_.begin(), hooks_.end(), sameControlBlock)) {
        CORE_LOG_E("%s hook is already registered", __func__);
        return;
    }
    hooks_.push_back(hook);
}

void FrameHookRegistry::Unregister(const std::weak_ptr<IFrameHook>& hook)
{
    // Match by control block (owner_before) so an already-expired hook can
    // still be removed; safe to call multiple times.
    hooks_.erase(std::remove_if(hooks_.begin(), hooks_.end(),
        [&hook](const std::weak_ptr<IFrameHook>& entry) {
            return !hook.owner_before(entry) && !entry.owner_before(hook);
        }), hooks_.end());
}

void FrameHookRegistry::PreRenderAll(RENDER_NS::IRenderContext& ctx)
{
    for (auto it = hooks_.begin(); it != hooks_.end();) {
        if (auto hook = it->lock()) {
            hook->PreRender(ctx);
            ++it;
        } else {
            // Hook was destroyed without Unregister(): prune the stale entry.
            it = hooks_.erase(it);
        }
    }
}

void FrameHookRegistry::PostRenderAll(RENDER_NS::IRenderContext& ctx)
{
    for (auto it = hooks_.begin(); it != hooks_.end();) {
        if (auto hook = it->lock()) {
            hook->PostRender(ctx);
            ++it;
        } else {
            it = hooks_.erase(it);
        }
    }
}

} // namespace OHOS::Render3D
