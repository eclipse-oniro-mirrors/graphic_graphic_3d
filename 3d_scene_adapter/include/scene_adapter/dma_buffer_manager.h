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

#ifndef OHOS_RENDER_3D_DMA_BUFFER_MANAGER_H
#define OHOS_RENDER_3D_DMA_BUFFER_MANAGER_H

#include <atomic>
#include <cstdint>

namespace OHOS::Render3D {
class DmaBufferManager {
public:
    static DmaBufferManager& GetInstance()
    {
        static DmaBufferManager instance;
        return instance;
    }

    bool AcquireQuota(size_t bytes)
    {
        size_t current = currentUsage_.load(std::memory_order_relaxed);
        if (current + bytes > threshold_) {
            return false;
        }
        currentUsage_.fetch_add(bytes, std::memory_order_relaxed);
        return true;
    }

    // Called when a DMA buffer is freed (PixelMap destroyed via custom deleter).
    void ReleaseQuota(size_t bytes)
    {
        currentUsage_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    size_t GetCurrentUsage() const
    {
        return currentUsage_.load(std::memory_order_relaxed);
    }

    size_t GetThreshold() const
    {
        return threshold_;
    }

    void SetThreshold(size_t bytes)
    {
        threshold_ = bytes;
    }

private:
    DmaBufferManager() = default;
    ~DmaBufferManager() = default;
    DmaBufferManager(const DmaBufferManager&) = delete;
    DmaBufferManager& operator=(const DmaBufferManager&) = delete;

    // 3 GB threshold - leaves headroom below the 4 GB dmabuf_monitor kill limit.
    std::atomic<size_t> threshold_{3ULL * 1024 * 1024 * 1024};
    std::atomic<size_t> currentUsage_{0};
};

}  // namespace OHOS::Render3D

#endif  // OHOS_RENDER_3D_DMA_BUFFER_MANAGER_H
