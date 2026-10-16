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

#include "scene_adapter/frame_signal_scope.h"

#include "core/log.h"
#include "render/device/intf_device.h"
#include "render/util/intf_render_util.h"

namespace OHOS::Render3D {

FrameSignalScope::FrameSignalScope(RENDER_NS::RenderHandleReference handle)
    : handle_(handle)
{
}

FrameSignalScope::~FrameSignalScope() = default;

void FrameSignalScope::Register(RENDER_NS::IRenderContext& ctx)
{
    auto& renderFrameUtil = ctx.GetRenderUtil().GetRenderFrameUtil();
    const auto backend = ctx.GetDevice().GetBackendType();

    RENDER_NS::IRenderFrameUtil::SignalData signalData;
    signalData.signaled = false;
    signalData.handle = handle_;

    if (backend == RENDER_NS::DeviceBackendType::VULKAN) {
        signalData.signalResourceType = RENDER_NS::IRenderFrameUtil::SignalResourceType::GPU_SEMAPHORE;
    } else if (backend == RENDER_NS::DeviceBackendType::OPENGLES) {
        signalData.signalResourceType = RENDER_NS::IRenderFrameUtil::SignalResourceType::GPU_FENCE;
    } else {
        CORE_LOG_E("%s unsupported backend type: %d", __func__, static_cast<int>(backend));
        return;
    }
    renderFrameUtil.AddGpuSignal(signalData);
}

OHOS::sptr<OHOS::SyncFence> FrameSignalScope::TryExportFence(RENDER_NS::IRenderContext& ctx)
{
    using ExportResult = RENDER_NS::IRenderFrameUtil::ExportResult;

    auto& renderFrameUtil = ctx.GetRenderUtil().GetRenderFrameUtil();
    // The util owns the backend-specific export details (semaphore fd on
    // Vulkan, native fence fd on GLES) and the export-once bookkeeping.
    const RENDER_NS::IRenderFrameUtil::NativeFence fence = renderFrameUtil.ExportFrameGpuSignal(handle_);
    switch (fence.result) {
        case ExportResult::EXPORTED:
            // Wrap the fd right at the ownership boundary: SyncFence's
            // UniqueFd takes over the close, so no caller ever closes it
            // manually (the last sptr reference drop closes the fd).
            return OHOS::sptr<OHOS::SyncFence>(new OHOS::SyncFence(fence.nativeFenceFd));
        case ExportResult::ALREADY_SIGNALED:
            // The signal already completed, there is nothing left to wait on.
            return OHOS::SyncFence::INVALID_FENCE;
        case ExportResult::NOT_SIGNALED:
            // Registered but never consumed (idle frame), or an unknown handle:
            // nothing to export.
            CORE_LOG_E("%s not signaled this frame -> invalid fence", __func__);
            return OHOS::SyncFence::INVALID_FENCE;
        case ExportResult::NOT_SUPPORTED:
        case ExportResult::FAILED:
            // Per the ExportFrameGpuSignal contract: with NOT_SUPPORTED or
            // FAILED synchronize on the CPU before releasing the buffer the
            // signal guards. An invalid fence would let consumers reuse the
            // buffer while the GPU may still be writing to it.
            CORE_LOG_E("%s export failed (%u), synchronize with WaitForIdle", __func__,
                static_cast<uint32_t>(fence.result));
            ctx.GetDevice().WaitForIdle();
            return OHOS::SyncFence::INVALID_FENCE;
        default:
            CORE_LOG_E("%s unexpected export result (%u)", __func__, static_cast<uint32_t>(fence.result));
            return OHOS::SyncFence::INVALID_FENCE;
    }
}

AcquireFenceWaitResult WaitAcquireFence(const BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& ctx,
    const OHOS::sptr<OHOS::SyncFence>& acquireFence, uint32_t timeoutMs)
{
    if (!acquireFence || !acquireFence->IsValid()) {
        CORE_LOG_E("%s acquireFence is null", __func__);
        return AcquireFenceWaitResult::SKIPPED;
    }
    if (!ctx) {
        CORE_LOG_E("%s ctx is null, falling back to CPU wait", __func__);
    }
    // Duplicate the fd before handing it to the renderer: the SyncFence's
    // UniqueFd owns the original fd and closes it when the SyncFence dies.
    // AddGpuWait takes over the duplicate only on ACCEPTED; in every other
    // case this function keeps full ownership and closes it below.
    int32_t fd = acquireFence->Dup();
    if (ctx && fd >= 0) {
        auto& renderFrameUtil = ctx->GetRenderUtil().GetRenderFrameUtil();
        RENDER_NS::IRenderFrameUtil::WaitData waitData;
        waitData.nativeFenceFd = fd;
        if (renderFrameUtil.AddGpuWait(waitData) == RENDER_NS::IRenderFrameUtil::WaitResult::ACCEPTED) {
            // The renderer owns the fd now and gates the next frame that
            // submits GPU work on it: the GPU waits on the fence, or, if the
            // import fails, the render thread waits on the CPU before
            // submitting. Either way the frame is synchronized, so no
            // blocking wait is needed here.
            return AcquireFenceWaitResult::GPU_WAITED;
        }
        // INVALID_FD or NOT_SUPPORTED: the caller keeps the fd, CPU fallback.
    }
    if (fd >= 0) {
        // The wait was not accepted, so the fd was not consumed: close the
        // duplicate here. The CPU fallback below waits on the SyncFence's
        // own fd.
        close(fd);
    }
    // CPU fallback: block until the producer finishes writing or timeout.
    int32_t ret = acquireFence->Wait(timeoutMs);
    if (ret != 0) {
        CORE_LOG_E("%s wait fence error: %d", __func__, ret);
        return AcquireFenceWaitResult::CPU_WAIT_FAILED;
    }
    return AcquireFenceWaitResult::CPU_WAITED;
}

} // namespace OHOS::Render3D
