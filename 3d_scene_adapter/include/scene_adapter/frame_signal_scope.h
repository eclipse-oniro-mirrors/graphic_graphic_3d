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

#ifndef OHOS_RENDER_3D_FRAME_SIGNAL_SCOPE_H
#define OHOS_RENDER_3D_FRAME_SIGNAL_SCOPE_H

#include <cstdint>

#include <base/containers/shared_ptr.h>
#include <refbase.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>
#include <render/util/intf_render_frame_util.h>
#include <sync_fence.h>

namespace OHOS::Render3D {

// Acquire fence CPU wait timeout (ms).
inline constexpr uint32_t ACQUIRE_FENCE_WAIT_TIMEOUT_MS = 5000;

// Return result of WaitAcquireFence().
enum class AcquireFenceWaitResult : uint32_t {
    GPU_WAITED,       // GPU-side wait succeeded
    CPU_WAITED,       // CPU fallback wait succeeded
    CPU_WAIT_FAILED,  // CPU fallback wait failed
    SKIPPED,          // fence is null, no wait needed
};

// Wait on the acquire fence, preferring a GPU-side wait via
// IRenderFrameUtil::AddGpuWait (Vulkan: semaphore/timeline import + queue
// wait; GLES: EGLSync import), with CPU SyncFence::Wait as fallback.
// The GPU wait is non-blocking: it hands the fence fd to the renderer, which
// gates the next frame that submits GPU work on it. Only when the wait is
// not accepted (null context, unusable or unsupported descriptor) does this
// block on the CPU.
// @param ctx Render context; null falls back to CPU wait.
// @param acquireFence Fence to wait on; null returns SKIPPED.
// @param timeoutMs CPU fallback timeout, defaults to ACQUIRE_FENCE_WAIT_TIMEOUT_MS.
// @return Wait result, see AcquireFenceWaitResult.
AcquireFenceWaitResult WaitAcquireFence(const BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& ctx,
    const OHOS::sptr<OHOS::SyncFence>& acquireFence, uint32_t timeoutMs = ACQUIRE_FENCE_WAIT_TIMEOUT_MS);

// Two-phase GPU signal sequence that produces a release fence fd:
//   Register(ctx) -> RenderDeferredFrame() -> TryExportFd(ctx)
// Two-phase because a Vulkan sync fd can only be exported from a semaphore
// with a pending signal (i.e. after submit).
// The render context is passed per phase instead of being stored: a scope
// can outlive the context it was created under (e.g. PreRender without a
// matching PostRender when teardown races the render loop), and a stored
// reference would dangle in that window. Each phase must receive the
// render context of the frame it runs in; the scope itself only carries
// the handle, which is reference-counted and stays valid on its own.
class FrameSignalScope {
public:
    // @param handle Signal handle; its id must be unique among all scopes
    // coexisting in one frame, or TryExportFd() may export another owner's
    // semaphore.
    explicit FrameSignalScope(RENDER_NS::RenderHandleReference handle);
    ~FrameSignalScope();

    FrameSignalScope(const FrameSignalScope&) = delete;
    FrameSignalScope& operator=(const FrameSignalScope&) = delete;

    // Pre-submit: registers a GPU_SEMAPHORE (Vulkan) or GPU_FENCE (GLES)
    // signal via AddGpuSignal. Must be called before RenderDeferredFrame.
    // @param ctx Render context of the frame about to be submitted.
    void Register(RENDER_NS::IRenderContext& ctx);

    // Post-submit: exports the signaled signal data as a sync fd via
    // IRenderFrameUtil::ExportFrameGpuSignal.
    // Returns -1 if the signal was not consumed (idle frame), already
    // signaled, or export failed; on an unsupported or failed export it
    // synchronizes with IDevice::WaitForIdle() before returning -1, so the
    // buffer the signal guards is safe to release either way.
    // @param ctx Render context of the frame that was submitted.
    int32_t TryExportFd(RENDER_NS::IRenderContext& ctx);

    // Signal handle; lets the caller pair the exported fd with the matching
    // buffer entry via SurfaceBufferOwner::SetReleaseFenceFd().
    const RENDER_NS::RenderHandleReference& GetHandle() const { return handle_; }

private:
    // Signal handle; ExportFrameGpuSignal() in TryExportFd() looks up the
    // frame's signal data by its id.
    RENDER_NS::RenderHandleReference handle_;
};

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_FRAME_SIGNAL_SCOPE_H
