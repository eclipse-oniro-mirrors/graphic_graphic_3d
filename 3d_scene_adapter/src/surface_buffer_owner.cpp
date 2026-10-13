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

#include "scene_adapter/surface_buffer_owner.h"

#include "core/log.h"

namespace OHOS::Render3D {

SurfaceBufferOwner::SurfaceBufferOwner(OHOS::sptr<OHOS::IConsumerSurface> surface)
    : surface_(surface)
{
}

SurfaceBufferOwner::~SurfaceBufferOwner()
{
    ReleaseAll();
}

void SurfaceBufferOwner::SetSurface(OHOS::sptr<OHOS::IConsumerSurface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Release everything cached for the old surface first, under the same
    // lock so nothing can be cached for the old surface in between.
    ReleaseAllLocked();
    surface_ = surface;
}

OHOS::sptr<OHOS::IConsumerSurface> SurfaceBufferOwner::GetSurface() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return surface_;
}

bool SurfaceBufferOwner::Acquire(OHOS::sptr<OHOS::SurfaceBuffer>& outBuffer,
                                 OHOS::sptr<OHOS::SyncFence>& outAcquireFence,
                                 int64_t& outTimestamp, OHOS::Rect& outDamage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!surface_) {
        CORE_LOG_E("%s invalid consumer surface", __func__);
        return false;
    }

    outAcquireFence = OHOS::SyncFence::INVALID_FENCE;
    outTimestamp = 0;
    outDamage = {0, 0, 0, 0};

    auto success = surface_->AcquireBuffer(outBuffer, outAcquireFence, outTimestamp, outDamage);
    if (success != OHOS::SURFACE_ERROR_OK || outBuffer == nullptr ||
        outAcquireFence == nullptr || !outAcquireFence->IsValid()) {
        CORE_LOG_E("%s acquire failed: %d", __func__, success);
        return false;
    }
    return true;
}

void SurfaceBufferOwner::ReleaseEntryLocked(BASE_NS::shared_ptr<BufferCacheEntry> entry)
{
    if (!surface_ || !entry) {
        CORE_LOG_E("%s invalid consumer surface or input", __func__);
        return;
    }
    auto surfaceBuffer = entry->GetSurfaceBuffer();
    if (!surfaceBuffer) {
        CORE_LOG_E("%s surfaceBuffer is nullptr", __func__);
        return;
    }
    // The entry already holds the release fence as a SyncFence (it was
    // wrapped at export time in TryExportFd); pass it on directly. A fence
    // that was never set stays INVALID_FENCE (entry never rendered).
    surface_->ReleaseBuffer(surfaceBuffer, entry->ConsumeReleaseFence());
}

void SurfaceBufferOwner::Cache(BASE_NS::shared_ptr<BufferCacheEntry> entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!entry) {
        CORE_LOG_E("%s invalid input", __func__);
        return;
    }
    // Release all but the last member before tail-inserting (see the class
    // comment for the eviction timing).
    if (cache_.size() > 1) {
        auto lastIt = std::prev(cache_.end());
        for (auto it = cache_.begin(); it != lastIt;) {
            if (*it) {
                ReleaseEntryLocked(*it);
            }
            it = cache_.erase(it);
        }
    }
    // Tail-insert the new entry.
    cache_.push_back(entry);
}

void SurfaceBufferOwner::ReleaseImmediate(OHOS::sptr<OHOS::SurfaceBuffer> buffer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (surface_ && buffer) {
        surface_->ReleaseBuffer(buffer, OHOS::SyncFence::INVALID_FENCE);
    }
}

void SurfaceBufferOwner::ReleaseAllLocked()
{
    // At teardown release with INVALID_FENCE; any stored release fence is
    // closed by its SyncFence when cache_.clear() drops the last reference.
    // The whole drain runs under mutex_ (held by the caller) so a concurrent
    // SetSurface() cannot swap surface_ between the entries being released.
    if (surface_) {
        for (auto& entry : cache_) {
            if (!entry) {
                continue;
            }
            auto surfaceBuffer = entry->GetSurfaceBuffer();
            if (surfaceBuffer) {
                surface_->ReleaseBuffer(surfaceBuffer, OHOS::SyncFence::INVALID_FENCE);
            }
        }
    }
    cache_.clear();
}

void SurfaceBufferOwner::ReleaseAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ReleaseAllLocked();
}

bool SurfaceBufferOwner::GetLatestSignalHandle(RENDER_NS::RenderHandleReference& outHandle) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    // The latest entry holds the buffer the next frame samples; its handle
    // is re-registered every frame until the entry is evicted.
    if (cache_.empty() || !cache_.back()) {
        CORE_LOG_E("%s invalid cache", __func__);
        return false;
    }
    outHandle = cache_.back()->GetSignalHandle();
    return true;
}

void SurfaceBufferOwner::SetReleaseFence(const RENDER_NS::RenderHandleReference& handle,
    const OHOS::sptr<OHOS::SyncFence>& fence)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (fence == nullptr || !fence->IsValid()) {
        // No fence this frame (idle frame or export failure): keep any
        // previously stored fence, it still covers the last GPU work.
        CORE_LOG_E("%s invalid fence", __func__);
        return;
    }
    // Attach the fence (ownership shared via sptr) to the entry whose signal
    // handle matches.
    for (auto& entry : cache_) {
        if (entry && entry->GetSignalHandle().GetHandle().id == handle.GetHandle().id) {
            entry->SetReleaseFence(fence);
            return;
        }
    }
    // No matching entry — the local sptr is the last reference, the fence
    // closes its own fd on destruction; nothing to do.
    CORE_LOG_E("%s no matching entry, fence dropped (fd auto-closed)", __func__);
}

} // namespace OHOS::Render3D
