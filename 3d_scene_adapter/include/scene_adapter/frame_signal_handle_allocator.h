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

#ifndef OHOS_RENDER_3D_FRAME_SIGNAL_HANDLE_ALLOCATOR_H
#define OHOS_RENDER_3D_FRAME_SIGNAL_HANDLE_ALLOCATOR_H

#include <atomic>

#include <render/resource_handle.h>

namespace OHOS::Render3D {

/// Allocates unique signal handle ids for FrameSignalScope instances.
///
/// Process-wide singleton: one monotonic counter backs all signal handles,
/// so ids never collide in the per-frame GetFrameGpuSignalData() list (a
/// collision would export the wrong owner's semaphore in TryExportFd()).
/// The high byte (0xBB) marks the ids as synthetic, outside the range of
/// real GPU resources.
class FrameSignalHandleAllocator {
public:
    /// Access the process-wide singleton. Thread-safe.
    static FrameSignalHandleAllocator& Instance()
    {
        static FrameSignalHandleAllocator instance;
        return instance;
    }

    FrameSignalHandleAllocator(const FrameSignalHandleAllocator&) = delete;
    FrameSignalHandleAllocator& operator=(const FrameSignalHandleAllocator&) = delete;
    FrameSignalHandleAllocator(FrameSignalHandleAllocator&&) = delete;
    FrameSignalHandleAllocator& operator=(FrameSignalHandleAllocator&&) = delete;

    /// Allocate a unique signal handle. Thread-safe.
    RENDER_NS::RenderHandleReference Alloc()
    {
        // High byte is tag; low 56 bits is global monotonic counter.
        const uint64_t id = FRAME_SIGNAL_HANDLE_TAG |
            (nextId_.fetch_add(1, std::memory_order_relaxed) & FRAME_SIGNAL_OWNER_ID_MASK);
        return RENDER_NS::RenderHandleReference{{id}, {}};
    }

private:
    FrameSignalHandleAllocator() = default;
    ~FrameSignalHandleAllocator() = default;

    /// High-byte tag marking ids as synthetic.
    static constexpr uint64_t FRAME_SIGNAL_HANDLE_TAG{0xBB00000000000000ULL};
    /// Mask for the monotonic counter in the low 56 bits.
    static constexpr uint64_t FRAME_SIGNAL_OWNER_ID_MASK{0x00FFFFFFFFFFFFFFULL};

    std::atomic<uint64_t> nextId_{1};
};

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_FRAME_SIGNAL_HANDLE_ALLOCATOR_H
