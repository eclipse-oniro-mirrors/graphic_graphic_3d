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

#include "util/native_fence_fd.h"

#if RENDER_HAS_NATIVE_FENCE_FD
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
namespace {
#if RENDER_HAS_NATIVE_FENCE_FD
// a signaled sync file polls readable, a hang-up or an error without POLLIN never signals
bool PollNativeFenceFd(const int32_t fd)
{
    pollfd pfd{fd, POLLIN, 0};
    int ret = -1;
    do {
        ret = poll(&pfd, 1U, static_cast<int>(NATIVE_FENCE_CPU_WAIT_TIMEOUT_MS));
    } while ((ret < 0) && (errno == EINTR));
    if (ret == 0) {
        PLUGIN_LOG_W("native fence did not signal within %u ms, the frame is not synchronized against it",
            NATIVE_FENCE_CPU_WAIT_TIMEOUT_MS);
        return false;
    }
    if ((ret < 0) || ((static_cast<uint32_t>(pfd.revents) & POLLIN) == 0U)) {
        PLUGIN_LOG_W("native fence wait failed (revents 0x%x), the frame is not synchronized against it",
            static_cast<uint32_t>(pfd.revents));
        return false;
    }
    return true;
}
#endif
}  // namespace

void CloseNativeFenceFd(const int32_t fd)
{
#if RENDER_HAS_NATIVE_FENCE_FD
    if (fd >= 0) {
        close(fd);
    }
#else
    PLUGIN_UNUSED(fd);
#endif
}

void CpuWaitNativeFence(IRenderFrameUtil::WaitData& waitData)
{
    if (waitData.nativeFenceFd < 0) {
        return;
    }
#if RENDER_HAS_NATIVE_FENCE_FD
    const bool signaled = PollNativeFenceFd(waitData.nativeFenceFd);
#else
    const bool signaled = false;
#endif
    waitData.status = signaled ? IRenderFrameUtil::WaitStatus::CPU_WAITED : IRenderFrameUtil::WaitStatus::IMPORT_FAILED;
    CloseNativeFenceFd(waitData.nativeFenceFd);
    waitData.nativeFenceFd = -1;
}
RENDER_END_NAMESPACE()
