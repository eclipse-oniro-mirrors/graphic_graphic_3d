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

#ifndef OHOS_RENDER_3D_SURFACE_BUFFER_OWNER_H
#define OHOS_RENDER_3D_SURFACE_BUFFER_OWNER_H

#include <cstdint>
#include <list>
#include <mutex>
#include <unistd.h>

#include <base/containers/shared_ptr.h>
#include <iconsumer_surface.h>
#include <surface_buffer.h>
#include <surface_utils.h>
#include <sync_fence.h>

#include <render/intf_render_context.h>
#include <render/resource_handle.h>

namespace OHOS::Render3D {

/// One acquired surface buffer plus the artifacts that pair it with its
/// release fence: a signal handle bound 1:1 at construction time (allocated
/// once in ProcessBufferAvailable, reused by every PreRender/PostRender cycle
/// while this buffer lives) and the fd exported in PostRender (matched by
/// handle, ownership transferred). Storing them together pairs each fd with
/// the correct buffer on release.
class BufferCacheEntry {
public:
    BufferCacheEntry(OHOS::sptr<OHOS::SurfaceBuffer> surfaceBuffer,
        RENDER_NS::RenderHandleReference signalHandle)
        : surfaceBuffer_(surfaceBuffer), signalHandle_(signalHandle)
    {
    }

    ~BufferCacheEntry()
    {
        if (releaseFenceFd_ >= 0) {
            close(releaseFenceFd_);
        }
    }

    BufferCacheEntry(const BufferCacheEntry&) = delete;
    BufferCacheEntry& operator=(const BufferCacheEntry&) = delete;

    OHOS::sptr<OHOS::SurfaceBuffer> GetSurfaceBuffer() const { return surfaceBuffer_; }
    const RENDER_NS::RenderHandleReference& GetSignalHandle() const { return signalHandle_; }

    /// Store the release fence fd. Takes ownership of fd (no dup; the entry
    /// closes or releases it). A negative fd is ignored: an idle frame
    /// exports no fence and the previously stored fd still covers the last
    /// real GPU work on this buffer.
    void SetReleaseFenceFd(int32_t fd)
    {
        if (fd < 0) {
            return;
        }
        if (releaseFenceFd_ >= 0) {
            close(releaseFenceFd_);
        }
        releaseFenceFd_ = fd;
    }

    /// Consume the release fence fd (transfers ownership, sets to -1).
    int32_t ConsumeReleaseFenceFd()
    {
        int32_t fd = releaseFenceFd_;
        releaseFenceFd_ = -1;
        return fd;
    }

private:
    OHOS::sptr<OHOS::SurfaceBuffer> surfaceBuffer_{nullptr};
    /// Signal handle bound 1:1 to this buffer at construction; pairs the
    /// buffer with its release fence fd.
    RENDER_NS::RenderHandleReference signalHandle_;
    int32_t releaseFenceFd_{-1};
};

/// Owns the buffer lifecycle (acquire, cache, release) for one consumer
/// surface under a single mutex.
///
/// The cache holds at most two entries: on arrival, Cache() releases all
/// but the last entry, then tail-inserts the new one. This ensures the GPU
/// is done sampling an old buffer before the producer overwrites it.
class SurfaceBufferOwner {
public:
    explicit SurfaceBufferOwner(OHOS::sptr<OHOS::IConsumerSurface> surface);
    ~SurfaceBufferOwner();

    SurfaceBufferOwner(const SurfaceBufferOwner&) = delete;
    SurfaceBufferOwner& operator=(const SurfaceBufferOwner&) = delete;

    /// Swap the consumer surface (releasing everything cached for the old one).
    void SetSurface(OHOS::sptr<OHOS::IConsumerSurface> surface);

    /// Access the consumer surface.
    OHOS::sptr<OHOS::IConsumerSurface> GetSurface() const;

    /// Acquire a buffer. Returns false on failure or when no surface is set.
    bool Acquire(OHOS::sptr<OHOS::SurfaceBuffer>& outBuffer,
                 OHOS::sptr<OHOS::SyncFence>& outAcquireFence,
                 int64_t& outTimestamp, OHOS::Rect& outDamage);

    /// Cache a new entry: release all but the last existing member, then
    /// tail-insert. See the class comment for the eviction timing.
    void Cache(BASE_NS::shared_ptr<BufferCacheEntry> entry);

    /// Release a buffer immediately (early-return paths before caching).
    void ReleaseImmediate(OHOS::sptr<OHOS::SurfaceBuffer> buffer);

    /// Drain the cache and release everything. Used at teardown.
    void ReleaseAll();

    /// Get the signal handle bound to the latest cached entry (the buffer the
    /// next frame samples). PreRender re-registers this handle every frame.
    /// Returns false when no entry is cached.
    bool GetLatestSignalHandle(RENDER_NS::RenderHandleReference& outHandle) const;

    /// Store the release fence fd on the entry whose signal handle matches.
    /// Ownership of fd transfers to the entry (no dup). A negative fd keeps
    /// the entry's previously stored fd.
    void SetReleaseFenceFd(const RENDER_NS::RenderHandleReference& handle, int32_t fd);

private:
    /// Release one entry with its release fence fd (or INVALID_FENCE).
    /// Caller must hold mutex_.
    void ReleaseEntryLocked(BASE_NS::shared_ptr<BufferCacheEntry> entry);

    /// Drain the cache and release everything with INVALID_FENCE; any
    /// stored release fence fd is closed by ~BufferCacheEntry() when the
    /// entries are destroyed. Caller must hold mutex_; keeping the surface_
    /// reads and the ReleaseBuffer() calls atomic prevents a concurrent
    /// SetSurface() from releasing old-surface entries to the new surface.
    void ReleaseAllLocked();

    mutable std::mutex mutex_;
    OHOS::sptr<OHOS::IConsumerSurface> surface_ = nullptr;
    /// Cached entries; holds at most two at any time. See the class comment.
    std::list<BASE_NS::shared_ptr<BufferCacheEntry>> cache_;
};
} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_SURFACE_BUFFER_OWNER_H
