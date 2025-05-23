/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H
#define VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "nodecontext/node_context_descriptor_set_manager.h"

RENDER_BEGIN_NAMESPACE()
struct RenderPassBeginInfo;
class GpuResourceManager;

struct LowLevelDescriptorSetVk {
    VkDescriptorSet descriptorSet { VK_NULL_HANDLE };
    // NOTE: descriptorSetLayout could be only one for buffering descriptor sets
    VkDescriptorSetLayout descriptorSetLayout { VK_NULL_HANDLE };

    enum DescriptorSetLayoutFlagBits : uint32_t {
        // immutable samplers bound in to the set (atm used only for ycbcr conversions which might need new psos)
        DESCRIPTOR_SET_LAYOUT_IMMUTABLE_SAMPLER_BIT = 0x00000001,
    };
    uint32_t flags { 0u };
    // has a bit set for the ones in 16 bindings that have immutable sampler
    uint16_t immutableSamplerBitmask { 0u };
};

struct LowLevelContextDescriptorPoolVk {
    // buffering count of 3 (max) is used often with vulkan triple buffering
    // gets the real used buffering count from the device
    static constexpr uint32_t MAX_BUFFERING_COUNT { DeviceConstants::MAX_BUFFERING_COUNT };

    VkDescriptorPool descriptorPool { VK_NULL_HANDLE };
    // additional descriptor pool for one frame descriptor sets with platform buffer bindings
    // reset and reserved every frame if needed, does not live through frames
    VkDescriptorPool additionalPlatformDescriptorPool { VK_NULL_HANDLE };

    struct DescriptorSetData {
        LowLevelDescriptorCounts descriptorCounts;
        // indexed with frame buffering index
        LowLevelDescriptorSetVk bufferingSet[MAX_BUFFERING_COUNT];

        // additional platform set used in case there are
        // e.g. ycbcr hwbufferw with immutable samplers and sampler conversion
        // one might change between normal combined_image_sampler every frame
        // these do not override the bufferingSet
        LowLevelDescriptorSetVk additionalPlatformSet;
    };
    BASE_NS::vector<DescriptorSetData> descriptorSets;
};

struct LowLevelContextDescriptorWriteDataVk {
    BASE_NS::vector<VkWriteDescriptorSet> writeDescriptorSets;
    BASE_NS::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
    BASE_NS::vector<VkDescriptorImageInfo> descriptorImageInfos;
    BASE_NS::vector<VkDescriptorImageInfo> descriptorSamplerInfos;
#if (RENDER_VULKAN_RT_ENABLED == 1)
    BASE_NS::vector<VkWriteDescriptorSetAccelerationStructureKHR> descriptorAccelInfos;
#endif

    uint32_t writeBindingCount { 0U };
    uint32_t bufferBindingCount { 0U };
    uint32_t imageBindingCount { 0U };
    uint32_t samplerBindingCount { 0U };

    void Clear()
    {
        writeBindingCount = 0U;
        bufferBindingCount = 0U;
        imageBindingCount = 0U;
        samplerBindingCount = 0U;

        writeDescriptorSets.clear();
        descriptorBufferInfos.clear();
        descriptorImageInfos.clear();
        descriptorSamplerInfos.clear();
#if (RENDER_VULKAN_RT_ENABLED == 1)
        descriptorAccelInfos.clear();
#endif
    }
};

struct PendingDeallocations {
    uint64_t frameIndex { 0 };
    LowLevelContextDescriptorPoolVk descriptorPool;
};

struct OneFrameDescriptorNeed {
    // the acceleration structure is the eleventh
    static constexpr uint32_t ACCELERATION_LOCAL_TYPE { 11U };
    static constexpr uint32_t DESCRIPTOR_ARRAY_SIZE = CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 2U;
    uint32_t descriptorCount[DESCRIPTOR_ARRAY_SIZE] { 0 };
};

class DescriptorSetManagerVk final : public DescriptorSetManager {
public:
    explicit DescriptorSetManagerVk(Device& device);
    ~DescriptorSetManagerVk() override;

    void BeginFrame() override;
    void BeginBackendFrame();

    void CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    bool UpdateDescriptorSetGpuHandle(const RenderHandle& handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

    const LowLevelDescriptorSetVk* GetDescriptorSet(const RenderHandle& handle) const;
    LowLevelContextDescriptorWriteDataVk& GetLowLevelDescriptorWriteData();

private:
    VkDevice vkDevice_;

    void ResizeDescriptorSetWriteData();

    BASE_NS::vector<PendingDeallocations> pendingDeallocations_;

    uint32_t bufferingCount_ { LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT };

    BASE_NS::vector<LowLevelContextDescriptorPoolVk> lowLevelDescriptorPools_;

    // use same vector, so we do not re-create the same with every reset
    // the memory allocation is small
    BASE_NS::vector<VkDescriptorPoolSize> descriptorPoolSizes_;

    // needs to be locked
    LowLevelContextDescriptorWriteDataVk lowLevelDescriptorWriteData_;
};

class NodeContextDescriptorSetManagerVk final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerVk(Device& device);
    ~NodeContextDescriptorSetManagerVk() override;

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;
    // Call when starting processing this node context. Creates one frame descriptor pool.
    void BeginBackendFrame();

    RenderHandle CreateDescriptorSet(
        BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    const LowLevelDescriptorSetVk* GetDescriptorSet(RenderHandle handle) const;
    LowLevelContextDescriptorWriteDataVk& GetLowLevelDescriptorWriteData();

    // deferred gpu descriptor set creation happens here
    bool UpdateDescriptorSetGpuHandle(RenderHandle handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

private:
    Device& device_;
    VkDevice vkDevice_;

    void ClearDescriptorSetWriteData();
    void ResizeDescriptorSetWriteData();

    uint32_t bufferingCount_ { 0 };
    LowLevelContextDescriptorPoolVk descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_COUNT];

    BASE_NS::vector<PendingDeallocations> pendingDeallocations_;

    OneFrameDescriptorNeed oneFrameDescriptorNeed_;

    // use same vector, so we do not re-create the same with every reset
    // the memory allocation is small
    BASE_NS::vector<VkDescriptorPoolSize> descriptorPoolSizes_;

    LowLevelContextDescriptorWriteDataVk lowLevelDescriptorWriteData_;

    uint32_t oneFrameDescSetGeneration_ { 0u };

#if (RENDER_VALIDATION_ENABLED == 1)
    static constexpr uint32_t MAX_ONE_FRAME_GENERATION_IDX { 16u };
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H
