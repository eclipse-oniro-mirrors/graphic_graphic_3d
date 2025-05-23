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
#include "node_context_descriptor_set_manager_vk.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/math/mathf.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/gpu_resource_util_vk.h"
#include "vulkan/gpu_sampler_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
inline constexpr uint32_t GetDescriptorIndex(const DescriptorType& dt)
{
    return (dt == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) ? (OneFrameDescriptorNeed::ACCELERATION_LOCAL_TYPE)
                                                               : static_cast<uint32_t>(dt);
}

inline constexpr VkDescriptorType GetDescriptorTypeVk(const uint32_t& idx)
{
    return (idx == OneFrameDescriptorNeed::ACCELERATION_LOCAL_TYPE) ? VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
                                                                    : static_cast<VkDescriptorType>(idx);
}

const VkSampler* GetSampler(const GpuResourceManager& gpuResourceMgr, const RenderHandle handle)
{
    if (const auto* gpuSampler = static_cast<GpuSamplerVk*>(gpuResourceMgr.GetSampler(handle)); gpuSampler) {
        return &(gpuSampler->GetPlatformData().sampler);
    } else {
        return nullptr;
    }
}

VkDescriptorPool CreatePoolFunc(
    const VkDevice device, const uint32_t maxSets, BASE_NS::vector<VkDescriptorPoolSize>& descriptorPoolSizes)
{
    constexpr VkDescriptorPoolCreateFlags descriptorPoolCreateFlags { 0 };
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
        nullptr,                                       // pNext
        descriptorPoolCreateFlags,                     // flags
        maxSets,                                       // maxSets
        (uint32_t)descriptorPoolSizes.size(),          // poolSizeCount
        descriptorPoolSizes.data(),                    // pPoolSizes
    };

    VkDescriptorPool descriptorPool { VK_NULL_HANDLE };
    VALIDATE_VK_RESULT(vkCreateDescriptorPool(device, // device
        &descriptorPoolCreateInfo,                    // pCreateInfo
        nullptr,                                      // pAllocator
        &descriptorPool));                            // pDescriptorPool

    return descriptorPool;
}

void DestroyPoolFunc(const VkDevice device, LowLevelContextDescriptorPoolVk& descriptorPool)
{
    for (auto& ref : descriptorPool.descriptorSets) {
        {
            // re-used with buffered descriptor sets, destroy only once
            auto& bufferingSetRef = ref.bufferingSet[0U];
            if (bufferingSetRef.descriptorSetLayout) {
                vkDestroyDescriptorSetLayout(device,     // device
                    bufferingSetRef.descriptorSetLayout, // descriptorSetLayout
                    nullptr);                            // pAllocator
                bufferingSetRef.descriptorSetLayout = VK_NULL_HANDLE;
            }
        }
        if (ref.additionalPlatformSet.descriptorSetLayout) {
            vkDestroyDescriptorSetLayout(device,               // device
                ref.additionalPlatformSet.descriptorSetLayout, // descriptorSetLayout
                nullptr);                                      // pAllocator
            ref.additionalPlatformSet.descriptorSetLayout = VK_NULL_HANDLE;
        }
    }
    descriptorPool.descriptorSets.clear();
    if (descriptorPool.descriptorPool) {
        vkDestroyDescriptorPool(device,    // device
            descriptorPool.descriptorPool, // descriptorPool
            nullptr);                      // pAllocator
        descriptorPool.descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorPool.additionalPlatformDescriptorPool) {
        vkDestroyDescriptorPool(device,                      // device
            descriptorPool.additionalPlatformDescriptorPool, // descriptorPool
            nullptr);                                        // pAllocator
        descriptorPool.additionalPlatformDescriptorPool = VK_NULL_HANDLE;
    }
}

void CreateCpuDescriptorSetData(const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings,
    const bool oneFrame, CpuDescriptorSet& newSet, LowLevelContextDescriptorPoolVk::DescriptorSetData& descSetData,
    OneFrameDescriptorNeed* oneFrameDescriptorNeed)
{
    uint32_t dynamicOffsetCount = 0;
    newSet.bindings.reserve(descriptorSetLayoutBindings.size());
    descSetData.descriptorCounts.writeDescriptorCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    for (const auto& refBinding : descriptorSetLayoutBindings) {
        // NOTE: sort from 0 to n
        newSet.bindings.push_back({ refBinding, {} });
        NodeContextDescriptorSetManager::IncreaseDescriptorSetCounts(
            refBinding, descSetData.descriptorCounts, dynamicOffsetCount);

#if (RENDER_VALIDATION_ENABLED == 1)
        if (refBinding.descriptorType > DescriptorType::CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT &&
            refBinding.descriptorType != DescriptorType::CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
            PLUGIN_LOG_E("RENDER_VALIDATION: Unhandled descriptor type");
        }
#endif
        const uint32_t descIndex = GetDescriptorIndex(refBinding.descriptorType);
        if (oneFrame && (descIndex < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE)) {
            PLUGIN_ASSERT(oneFrameDescriptorNeed);
            oneFrameDescriptorNeed->descriptorCount[descIndex] += refBinding.descriptorCount;
        }
    }
    newSet.buffers.resize(descSetData.descriptorCounts.bufferCount);
    newSet.images.resize(descSetData.descriptorCounts.imageCount);
    newSet.samplers.resize(descSetData.descriptorCounts.samplerCount);

    newSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);
}

void CreateGpuDescriptorSetFunc(const Device& device, const uint32_t bufferCount, const RenderHandle clientHandle,
    const CpuDescriptorSet& cpuDescriptorSet, LowLevelContextDescriptorPoolVk& descriptorPool, const string& debugName)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSet.bindings.size() > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
        PLUGIN_LOG_W("RENDER_VALIDATION: descriptor set binding count exceeds (max:%u, current:%u)",
            PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT,
            static_cast<uint32_t>(cpuDescriptorSet.bindings.size()));
    }
#endif
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(clientHandle);
    VkDescriptorBindingFlags descriptorBindingFlags[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
    const uint32_t bindingCount = Math::min(static_cast<uint32_t>(cpuDescriptorSet.bindings.size()),
        PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
    const bool hasPlatformBindings = cpuDescriptorSet.hasPlatformConversionBindings;
    const bool hasBindImmutableSamplers = cpuDescriptorSet.hasImmutableSamplers;
    uint16_t immutableSamplerBitmask = 0;
    const auto& gpuResourceMgr = static_cast<const GpuResourceManager&>(device.GetGpuResourceManager());
    // NOTE: if we cannot provide explicit flags that custom immutable sampler with conversion is needed
    // we should first loop through the bindings and check
    // normal hw buffers do not need any rebindings or immutable samplers
    for (uint32_t idx = 0; idx < bindingCount; ++idx) {
        const DescriptorSetLayoutBindingResource& cpuBinding = cpuDescriptorSet.bindings[idx];
        const auto descriptorType = (VkDescriptorType)cpuBinding.binding.descriptorType;
        const auto stageFlags = (VkShaderStageFlags)cpuBinding.binding.shaderStageFlags;
        const uint32_t bindingIdx = cpuBinding.binding.binding;
        const VkSampler* immutableSampler = nullptr;
        if (hasBindImmutableSamplers) {
            if (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                const auto& imgRef = cpuDescriptorSet.images[cpuBinding.resourceIndex].desc;
                if (imgRef.additionalFlags & CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT) {
                    immutableSampler = GetSampler(gpuResourceMgr, imgRef.resource.samplerHandle);
                    immutableSamplerBitmask |= (1 << bindingIdx);
                }
            } else if (descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                const auto& samRef = cpuDescriptorSet.samplers[cpuBinding.resourceIndex].desc;
                if (samRef.additionalFlags & CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT) {
                    immutableSampler = GetSampler(gpuResourceMgr, samRef.resource.handle);
                    immutableSamplerBitmask |= (1 << bindingIdx);
                }
            }
        } else if (hasPlatformBindings && (descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
            const RenderHandle handle = cpuDescriptorSet.images[cpuBinding.resourceIndex].desc.resource.handle;
            if (RenderHandleUtil::IsPlatformConversionResource(handle)) {
                if (const auto* gpuImage = static_cast<GpuImageVk*>(gpuResourceMgr.GetImage(handle)); gpuImage) {
                    const GpuImage::AdditionalFlags additionalFlags = gpuImage->GetAdditionalFlags();
                    immutableSampler = &(gpuImage->GetPlaformDataConversion().sampler);
                    if ((additionalFlags & GpuImage::AdditionalFlagBits::ADDITIONAL_PLATFORM_CONVERSION_BIT) &&
                        immutableSampler) {
                        immutableSamplerBitmask |= (1 << bindingIdx);
                    }
#if (RENDER_VALIDATION_ENABLED == 1)
                    if (!immutableSampler) {
                        PLUGIN_LOG_W("RENDER_VALIDATION: immutable sampler for platform conversion resource not found");
                    }
#endif
                }
            }
        }
        descriptorSetLayoutBindings[idx] = {
            bindingIdx,                         // binding
            descriptorType,                     // descriptorType
            cpuBinding.binding.descriptorCount, // descriptorCount
            stageFlags,                         // stageFlags
            immutableSampler,                   // pImmutableSamplers
        };
        // NOTE: partially bound is not used at the moment
        descriptorBindingFlags[idx] = 0U;
    }

    const auto& deviceVk = (const DeviceVk&)device;
    const VkDevice vkDevice = deviceVk.GetPlatformDataVk().device;

    const VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, // sType
        nullptr,                                                           // pNext
        bindingCount,                                                      // bindingCount
        descriptorBindingFlags,                                            // pBindingFlags
    };
    const bool dsiEnabled = deviceVk.GetCommonDeviceExtensions().descriptorIndexing;
    const void* pNextPtr = dsiEnabled ? (&descriptorSetLayoutBindingFlagsCreateInfo) : nullptr;
    // NOTE: update after bind etc. are not currently in use
    // descriptor set indexing is used with normal binding model
    constexpr VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags { 0U };
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
        pNextPtr,                                            // pNext
        descriptorSetLayoutCreateFlags,                      // flags
        bindingCount,                                        // bindingCount
        descriptorSetLayoutBindings,                         // pBindings
    };

    // used with all buffered sets
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VALIDATE_VK_RESULT(vkCreateDescriptorSetLayout(vkDevice, // device
        &descriptorSetLayoutCreateInfo,                      // pCreateInfo
        nullptr,                                             // pAllocator
        &descriptorSetLayout));                              // pSetLayout

    for (uint32_t idx = 0; idx < bufferCount; ++idx) {
        PLUGIN_ASSERT(!descriptorPool.descriptorSets.empty());

        LowLevelDescriptorSetVk newDescriptorSet;
        newDescriptorSet.descriptorSetLayout = descriptorSetLayout;
        newDescriptorSet.flags |=
            (immutableSamplerBitmask != 0) ? LowLevelDescriptorSetVk::DESCRIPTOR_SET_LAYOUT_IMMUTABLE_SAMPLER_BIT : 0u;
        newDescriptorSet.immutableSamplerBitmask = immutableSamplerBitmask;

        VkDescriptorPool descriptorPoolVk = VK_NULL_HANDLE;
        // for platform immutable set we use created additional descriptor pool (currently only used with ycbcr)
        const bool platImmutable = (hasPlatformBindings && (immutableSamplerBitmask != 0));
        if (platImmutable) {
            descriptorPoolVk = descriptorPool.additionalPlatformDescriptorPool;
        }
        if (!descriptorPoolVk) {
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
            if (platImmutable) {
                const auto errorType = "plat_immutable_hwbuffer_" + to_string(idx);
                PLUGIN_LOG_ONCE_E(
                    errorType, "Platform descriptor set not available, using regular (name:%s)", debugName.c_str());
            }
#endif
            descriptorPoolVk = descriptorPool.descriptorPool;
        }
        PLUGIN_ASSERT(descriptorPoolVk);
        const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // sType
            nullptr,                                        // pNext
            descriptorPoolVk,                               // descriptorPool
            1u,                                             // descriptorSetCount
            &newDescriptorSet.descriptorSetLayout,          // pSetLayouts
        };

        VALIDATE_VK_RESULT(vkAllocateDescriptorSets(vkDevice, // device
            &descriptorSetAllocateInfo,                       // pAllocateInfo
            &newDescriptorSet.descriptorSet));                // pDescriptorSets
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        GpuResourceUtil::DebugObjectNameVk(device, VK_OBJECT_TYPE_DESCRIPTOR_SET,
            VulkanHandleCast<uint64_t>(newDescriptorSet.descriptorSet),
            debugName + "_ds_" + to_string(arrayIndex) + "_" + to_string(idx));
#endif

        if (platImmutable) {
            descriptorPool.descriptorSets[arrayIndex].additionalPlatformSet = newDescriptorSet;
        } else {
            PLUGIN_ASSERT(descriptorPool.descriptorSets[arrayIndex].bufferingSet[idx].descriptorSet == VK_NULL_HANDLE);
            descriptorPool.descriptorSets[arrayIndex].bufferingSet[idx] = newDescriptorSet;
        }
        // NOTE: descriptor sets could be tagged with debug name
        // might be a bit overkill to do it always
#if (RENDER_VALIDATION_ENABLED == 1)
        if (newDescriptorSet.descriptorSet == VK_NULL_HANDLE) {
            PLUGIN_LOG_E("RENDER_VALIDATION: gpu descriptor set creation failed, ds node: %s, ds binding count: %u",
                debugName.c_str(), bindingCount);
        }
#endif
    }
}

const LowLevelDescriptorSetVk* GetDescriptorSetFunc(const CpuDescriptorSet& cpuDescriptorSet,
    const LowLevelContextDescriptorPoolVk::DescriptorSetData& descriptorSetData, const string& debugName,
    const RenderHandle& handle)
{
    const LowLevelDescriptorSetVk* set = nullptr;
    {
        // additional set is only used there are platform buffer bindings and additional set created
        const bool useAdditionalSet =
            (cpuDescriptorSet.hasPlatformConversionBindings && descriptorSetData.additionalPlatformSet.descriptorSet);
        if (useAdditionalSet) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (!descriptorSetData.additionalPlatformSet.descriptorSet) {
                PLUGIN_LOG_ONCE_E(debugName.c_str() + to_string(handle.id) + "_dsnu0",
                    "RENDER_VALIDATION: descriptor set not updated (handle:%" PRIx64 ")", handle.id);
            }
#endif
            set = &descriptorSetData.additionalPlatformSet;
        } else {
            const uint32_t bufferingIndex = cpuDescriptorSet.currentGpuBufferingIndex;
#if (RENDER_VALIDATION_ENABLED == 1)
            if (!descriptorSetData.bufferingSet[bufferingIndex].descriptorSet) {
                PLUGIN_LOG_ONCE_E(debugName.c_str() + to_string(handle.id) + "_dsn1",
                    "RENDER_VALIDATION: descriptor set not updated (handle:%" PRIx64 ")", handle.id);
            }
#endif
            set = &descriptorSetData.bufferingSet[bufferingIndex];
        }
    }

#if (RENDER_VALIDATION_ENABLED == 1)
    if (set) {
        if (set->descriptorSet == VK_NULL_HANDLE) {
            PLUGIN_LOG_ONCE_E(debugName.c_str() + to_string(handle.id) + "_dsnu2",
                "RENDER_VALIDATION: descriptor set has not been updated prior to binding");
            PLUGIN_LOG_ONCE_E(debugName.c_str() + to_string(handle.id) + "_dsnu3",
                "RENDER_VALIDATION: gpu descriptor set created? %u, descriptor set node: %s,"
                "buffer count: %u, "
                "image count: %u, sampler count: %u",
                (uint32_t)cpuDescriptorSet.gpuDescriptorSetCreated, debugName.c_str(),
                descriptorSetData.descriptorCounts.bufferCount, descriptorSetData.descriptorCounts.imageCount,
                descriptorSetData.descriptorCounts.samplerCount);
        }
    }
#endif

    return set;
}
} // namespace

DescriptorSetManagerVk::DescriptorSetManagerVk(Device& device)
    : DescriptorSetManager(device), vkDevice_ { ((const DevicePlatformDataVk&)device_.GetPlatformData()).device },
      bufferingCount_(
          Math::min(LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT, device_.GetCommandBufferingCount()))
{}

DescriptorSetManagerVk::~DescriptorSetManagerVk()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    uint32_t referenceCounts = 0U;
    for (const auto& baseRef : descriptorSets_) {
        if (baseRef) {
            for (const auto& ref : baseRef->data) {
                if (ref.renderHandleReference.GetRefCount() > 1) {
                    referenceCounts++;
                }
            }
        }
    }
    if (referenceCounts > 0U) {
        PLUGIN_LOG_W(
            "RENDER_VALIDATION: Not all references removed from global descriptor set handles (%u)", referenceCounts);
    }
#endif
    for (auto& ref : lowLevelDescriptorPools_) {
        DestroyPoolFunc(vkDevice_, ref);
    }
    for (auto& ref : pendingDeallocations_) {
        DestroyPoolFunc(vkDevice_, ref.descriptorPool);
    }
}

void DescriptorSetManagerVk::BeginFrame()
{
    DescriptorSetManager::BeginFrame();

    lowLevelDescriptorWriteData_.Clear();
}

void DescriptorSetManagerVk::BeginBackendFrame()
{
    // NOTE: ATM during backend there's no possiblity to lock for creation
    // -> we don't need locks here

    ResizeDescriptorSetWriteData();

    // clear aged descriptor pools
    if (!pendingDeallocations_.empty()) {
        // this is normally empty or only has single item
        const auto minAge = device_.GetCommandBufferingCount() + 1;
        const auto ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

        auto oldRes = std::partition(pendingDeallocations_.begin(), pendingDeallocations_.end(),
            [ageLimit](auto const& pd) { return pd.frameIndex >= ageLimit; });

        std::for_each(
            oldRes, pendingDeallocations_.end(), [this](auto& res) { DestroyPoolFunc(vkDevice_, res.descriptorPool); });
        pendingDeallocations_.erase(oldRes, pendingDeallocations_.end());
    }

    // handle pool creations and write locking
    // handle possible destruction
    PLUGIN_ASSERT(descriptorSets_.size() == lowLevelDescriptorPools_.size());
    for (uint32_t idx = 0; idx < descriptorSets_.size(); ++idx) {
        LowLevelContextDescriptorPoolVk& descriptorPool = lowLevelDescriptorPools_[idx];
        GlobalDescriptorSetBase* descriptorSetBase = descriptorSets_[idx].get();
        if (!descriptorSetBase) {
            continue;
        }
        bool destroyDescriptorSets = true;
        // if we have any descriptor sets in use we do not destroy the pool
        for (auto& ref : descriptorSetBase->data) {
            if (ref.renderHandleReference.GetRefCount() > 1) {
                destroyDescriptorSets = false;
            }
            ref.frameWriteLocked = false;
        }

        // destroy or create pool
        if (destroyDescriptorSets) {
            if (descriptorPool.descriptorPool || descriptorPool.additionalPlatformDescriptorPool) {
                PendingDeallocations pd;
                pd.descriptorPool.descriptorPool = exchange(descriptorPool.descriptorPool, VK_NULL_HANDLE);
                pd.descriptorPool.additionalPlatformDescriptorPool =
                    exchange(descriptorPool.additionalPlatformDescriptorPool, VK_NULL_HANDLE);
                pd.descriptorPool.descriptorSets = move(descriptorPool.descriptorSets);
                pd.frameIndex = device_.GetFrameCount();
                pendingDeallocations_.push_back(move(pd));
            }
            if (!descriptorSetBase->data.empty()) {
                const RenderHandle handle = descriptorSetBase->data[0U].renderHandleReference.GetHandle();
                // set handle (index location) to be available
                availableHandles_.push_back(handle);
            }
            // reset
            nameToIndex_.erase(descriptorSetBase->name);
            *descriptorSetBase = {};
        } else if (descriptorPool.descriptorPool == VK_NULL_HANDLE) {
            descriptorPoolSizes_.clear();
            descriptorPoolSizes_.reserve(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
            // all the the descriptor sets have the same amount of data
            if ((!descriptorSetBase->data.empty()) && (!descriptorPool.descriptorSets.empty())) {
                const auto descriptorSetCount = static_cast<uint32_t>(descriptorPool.descriptorSets.size());
                for (const auto& bindRef : descriptorSetBase->data[0U].cpuDescriptorSet.bindings) {
                    const auto& bind = bindRef.binding;
                    descriptorPoolSizes_.push_back(VkDescriptorPoolSize { (VkDescriptorType)bind.descriptorType,
                        bind.descriptorCount * descriptorSetCount * bufferingCount_ });
                }
                if (!descriptorPoolSizes_.empty()) {
                    descriptorPool.descriptorPool =
                        CreatePoolFunc(vkDevice_, descriptorSetCount * bufferingCount_, descriptorPoolSizes_);
                }
            }
        }
    }
}

void DescriptorSetManagerVk::CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    PLUGIN_ASSERT((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex]));
    PLUGIN_ASSERT(descriptorSets_[arrayIndex]->data.size() == descriptorSetCount);
    if ((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex])) {
        // resize based on cpu descriptor sets
        lowLevelDescriptorPools_.resize(descriptorSets_.size());
        PLUGIN_ASSERT(arrayIndex < lowLevelDescriptorPools_.size());

        LowLevelContextDescriptorPoolVk& descriptorPool = lowLevelDescriptorPools_[arrayIndex];
        // destroy old pool properly
        if (descriptorPool.descriptorPool || descriptorPool.additionalPlatformDescriptorPool) {
            PendingDeallocations pd;
            pd.descriptorPool.descriptorPool = exchange(descriptorPool.descriptorPool, VK_NULL_HANDLE);
            pd.descriptorPool.additionalPlatformDescriptorPool =
                exchange(descriptorPool.additionalPlatformDescriptorPool, VK_NULL_HANDLE);
            pd.descriptorPool.descriptorSets = move(descriptorPool.descriptorSets);
            pd.frameIndex = device_.GetFrameCount();
            pendingDeallocations_.push_back(move(pd));
        }
        descriptorPool.descriptorSets.clear();

        descriptorPool.descriptorSets.resize(descriptorSetCount);
        GlobalDescriptorSetBase* cpuData = descriptorSets_[arrayIndex].get();
        PLUGIN_ASSERT(cpuData->data.size() == descriptorPool.descriptorSets.size());
        for (uint32_t idx = 0; idx < descriptorSetCount; ++idx) {
            CpuDescriptorSet newSet;
            LowLevelContextDescriptorPoolVk::DescriptorSetData descSetData;
            CreateCpuDescriptorSetData(descriptorSetLayoutBindings, false, newSet, descSetData, nullptr);

            const uint32_t additionalIndex = idx;
            cpuData->data[additionalIndex].cpuDescriptorSet = move(newSet);
            // don't create the actual gpu descriptor sets yet
            descriptorPool.descriptorSets[additionalIndex] = descSetData;
        }
    }
}

bool DescriptorSetManagerVk::UpdateDescriptorSetGpuHandle(const RenderHandle& handle)
{
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::DESCRIPTOR_SET);

    // NOTE: this is called from the backend from a single thread and other threads cannot access the data
    // does not need to be locked

    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    GlobalDescriptorSetBase* baseSet = nullptr;
    if ((index < descriptorSets_.size()) && descriptorSets_[index]) {
        baseSet = descriptorSets_[index].get();
    }

    bool retValue = false;
    if (baseSet && (index < lowLevelDescriptorPools_.size())) {
        LowLevelContextDescriptorPoolVk& descriptorPool = lowLevelDescriptorPools_[index];
        const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);
        if ((additionalIndex < baseSet->data.size()) && (additionalIndex < descriptorPool.descriptorSets.size())) {
            CpuDescriptorSet& refCpuSet = baseSet->data[additionalIndex].cpuDescriptorSet;
            const RenderHandle indexHandle = RenderHandleUtil::CreateHandle(
                RenderHandleType::DESCRIPTOR_SET, additionalIndex); // using array index from handle

            // with platform buffer bindings descriptor set creation needs to be checked
            if (refCpuSet.hasPlatformConversionBindings) {
                // no buffering
                CreateGpuDescriptorSetFunc(device_, 1U, indexHandle, refCpuSet, descriptorPool, baseSet->name);
            } else if (!refCpuSet.gpuDescriptorSetCreated) { // deferred creation
                CreateGpuDescriptorSetFunc(
                    device_, bufferingCount_, indexHandle, refCpuSet, descriptorPool, baseSet->name);
                refCpuSet.gpuDescriptorSetCreated = true;
            }

            // at the moment should be always dirty when coming here from the backend update
            if (refCpuSet.isDirty) {
                refCpuSet.isDirty = false;
                // advance to next gpu descriptor set
                refCpuSet.currentGpuBufferingIndex = (refCpuSet.currentGpuBufferingIndex + 1) % bufferingCount_;
                retValue = true;
            }
        }
    }
    return retValue;
}

const LowLevelDescriptorSetVk* DescriptorSetManagerVk::GetDescriptorSet(const RenderHandle& handle) const
{
    // NOTE: this is called from the backend from a single thread and other threads cannot access the data
    // does not need to be locked

    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    GlobalDescriptorSetBase* baseSet = nullptr;
    if ((index < descriptorSets_.size()) && descriptorSets_[index]) {
        baseSet = descriptorSets_[index].get();
    }

    const LowLevelDescriptorSetVk* set = nullptr;
    if (baseSet && (index < lowLevelDescriptorPools_.size())) {
        const LowLevelContextDescriptorPoolVk& descriptorPool = lowLevelDescriptorPools_[index];
        const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);
        if ((additionalIndex < baseSet->data.size()) && (additionalIndex < descriptorPool.descriptorSets.size())) {
            const CpuDescriptorSet& refCpuSet = baseSet->data[additionalIndex].cpuDescriptorSet;
            if ((!descriptorPool.descriptorSets.empty()) && (additionalIndex < descriptorPool.descriptorSets.size())) {
                const RenderHandle indexHandle = RenderHandleUtil::CreateHandle(
                    RenderHandleType::DESCRIPTOR_SET, additionalIndex); // using array index from handle
                set = GetDescriptorSetFunc(
                    refCpuSet, descriptorPool.descriptorSets[additionalIndex], baseSet->name, indexHandle);
            }
        }
    }
    return set;
}

// needs to be locked when called
void DescriptorSetManagerVk::UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources)
{
    // needs to be locked when accessed from multiple threads

    lowLevelDescriptorWriteData_.writeBindingCount += static_cast<uint32_t>(bindingResources.bindings.size());

    lowLevelDescriptorWriteData_.bufferBindingCount += static_cast<uint32_t>(bindingResources.buffers.size());
    lowLevelDescriptorWriteData_.imageBindingCount += static_cast<uint32_t>(bindingResources.images.size());
    lowLevelDescriptorWriteData_.samplerBindingCount += static_cast<uint32_t>(bindingResources.samplers.size());
}

void DescriptorSetManagerVk::ResizeDescriptorSetWriteData()
{
    auto& descWd = lowLevelDescriptorWriteData_;
    if (descWd.writeBindingCount > 0U) {
        lowLevelDescriptorWriteData_.writeDescriptorSets.resize(descWd.writeBindingCount);
        lowLevelDescriptorWriteData_.descriptorBufferInfos.resize(descWd.bufferBindingCount);
        lowLevelDescriptorWriteData_.descriptorImageInfos.resize(descWd.imageBindingCount);
        lowLevelDescriptorWriteData_.descriptorSamplerInfos.resize(descWd.samplerBindingCount);
#if (RENDER_VULKAN_RT_ENABLED == 1)
        lowLevelDescriptorWriteData_.descriptorAccelInfos.resize(descWd.bufferBindingCount);
#endif
    }
}

LowLevelContextDescriptorWriteDataVk& DescriptorSetManagerVk::GetLowLevelDescriptorWriteData()
{
    return lowLevelDescriptorWriteData_;
}

NodeContextDescriptorSetManagerVk::NodeContextDescriptorSetManagerVk(Device& device)
    : NodeContextDescriptorSetManager(device), device_ { device },
      vkDevice_ { ((const DevicePlatformDataVk&)device_.GetPlatformData()).device },
      bufferingCount_(
          Math::min(LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT, device_.GetCommandBufferingCount()))
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (device_.GetCommandBufferingCount() > LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT) {
        PLUGIN_LOG_ONCE_W("device_command_buffering_count_desc_set_vk_buffering",
            "RENDER_VALIDATION: device command buffering count (%u) is larger than supported vulkan descriptor set "
            "buffering count (%u)",
            device_.GetCommandBufferingCount(), LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT);
    }
#endif
}

NodeContextDescriptorSetManagerVk::~NodeContextDescriptorSetManagerVk()
{
    DestroyPoolFunc(vkDevice_, descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC]);
    DestroyPoolFunc(vkDevice_, descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME]);
    for (auto& ref : pendingDeallocations_) {
        DestroyPoolFunc(vkDevice_, ref.descriptorPool);
    }
}

void NodeContextDescriptorSetManagerVk::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
    if (maxSets_ > 0) {
        auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        // usually there are less descriptor sets than max sets count
        // due to maxsets count has been calculated for single descriptors
        // (one descriptor set has multiple descriptors)
        const uint32_t reserveCount = maxSets_ / 2u; // questimate for possible max vector size;

        if (descriptorPool.descriptorPool) { // push for dealloation vec
            PendingDeallocations pd;
            pd.descriptorPool.descriptorPool = move(descriptorPool.descriptorPool);
            pd.descriptorPool.descriptorSets = move(descriptorPool.descriptorSets);
            pd.frameIndex = device_.GetFrameCount();
            pendingDeallocations_.push_back(move(pd));

            descriptorPool.descriptorSets.clear();
            descriptorPool.descriptorPool = VK_NULL_HANDLE;
        }

        descriptorPoolSizes_.clear();
        descriptorPoolSizes_.reserve(descriptorCounts.counts.size()); // max count reserve
        for (const auto& ref : descriptorCounts.counts) {
            if (ref.count > 0) {
                descriptorPoolSizes_.push_back(
                    VkDescriptorPoolSize { (VkDescriptorType)ref.type, ref.count * bufferingCount_ });
            }
        }

        if (!descriptorPoolSizes_.empty()) {
            descriptorPool.descriptorPool = CreatePoolFunc(vkDevice_, maxSets_ * bufferingCount_, descriptorPoolSizes_);
            descriptorPool.descriptorSets.reserve(reserveCount);
        }
    }
}

void NodeContextDescriptorSetManagerVk::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();

    ClearDescriptorSetWriteData();

    // clear aged descriptor pools
    if (!pendingDeallocations_.empty()) {
        // this is normally empty or only has single item
        const auto minAge = device_.GetCommandBufferingCount() + 1;
        const auto ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

        auto oldRes = std::partition(pendingDeallocations_.begin(), pendingDeallocations_.end(),
            [ageLimit](auto const& pd) { return pd.frameIndex >= ageLimit; });

        std::for_each(
            oldRes, pendingDeallocations_.end(), [this](auto& res) { DestroyPoolFunc(vkDevice_, res.descriptorPool); });
        pendingDeallocations_.erase(oldRes, pendingDeallocations_.end());
    }

    oneFrameDescriptorNeed_ = {};
    auto& oneFrameDescriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    if (oneFrameDescriptorPool.descriptorPool || oneFrameDescriptorPool.additionalPlatformDescriptorPool) {
        const auto descriptorSetCount = static_cast<uint32_t>(oneFrameDescriptorPool.descriptorSets.size());
        PendingDeallocations pd;
        pd.descriptorPool.descriptorPool = exchange(oneFrameDescriptorPool.descriptorPool, VK_NULL_HANDLE);
        pd.descriptorPool.additionalPlatformDescriptorPool =
            exchange(oneFrameDescriptorPool.additionalPlatformDescriptorPool, VK_NULL_HANDLE);
        pd.descriptorPool.descriptorSets = move(oneFrameDescriptorPool.descriptorSets);
        pd.frameIndex = device_.GetFrameCount();
        pendingDeallocations_.push_back(move(pd));

        oneFrameDescriptorPool.descriptorSets.reserve(descriptorSetCount);
    }
    oneFrameDescriptorPool.descriptorSets.clear();

    // we need to check through platform special format desriptor sets/pool
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    if (descriptorPool.additionalPlatformDescriptorPool) {
        PendingDeallocations pd;
        pd.descriptorPool.additionalPlatformDescriptorPool = move(descriptorPool.additionalPlatformDescriptorPool);
        // no buffering set
        pd.frameIndex = device_.GetFrameCount();
        pendingDeallocations_.push_back(move(pd));
        descriptorPool.additionalPlatformDescriptorPool = VK_NULL_HANDLE;
        // immediate desctruction of descriptor set layouts
        for (auto& descriptorSetRef : descriptorPool.descriptorSets) {
            if (descriptorSetRef.additionalPlatformSet.descriptorSetLayout) {
                vkDestroyDescriptorSetLayout(vkDevice_,                         // device
                    descriptorSetRef.additionalPlatformSet.descriptorSetLayout, // descriptorSetLayout
                    nullptr);                                                   // pAllocator
                descriptorSetRef.additionalPlatformSet.descriptorSetLayout = VK_NULL_HANDLE;
            }
        }
        auto& cpuDescriptorSet = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        for (auto& descriptorSetRef : cpuDescriptorSet) {
            descriptorSetRef.hasPlatformConversionBindings = false;
        }
    }

#if (RENDER_VALIDATION_ENABLED == 1)
    oneFrameDescSetGeneration_ = (oneFrameDescSetGeneration_ + 1) % MAX_ONE_FRAME_GENERATION_IDX;
#endif
}

void NodeContextDescriptorSetManagerVk::BeginBackendFrame()
{
    // resize vector data
    ResizeDescriptorSetWriteData();

    // reserve descriptors for descriptors sets that need platform special formats for one frame
    if (hasPlatformConversionBindings_) {
        const auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        uint32_t descriptorSetCount = 0u;
        uint32_t descriptorCounts[OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE] { 0 };
        for (const auto& cpuDescriptorSetRef : cpuDescriptorSets) {
            if (cpuDescriptorSetRef.hasPlatformConversionBindings) {
                descriptorSetCount++;
                for (const auto& bindingRef : cpuDescriptorSetRef.bindings) {
                    uint32_t descriptorCount = bindingRef.binding.descriptorCount;
                    const auto descTypeIndex = GetDescriptorIndex(bindingRef.binding.descriptorType);
                    if (descTypeIndex < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE) {
                        if ((bindingRef.binding.descriptorType ==
                                DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                            RenderHandleUtil::IsPlatformConversionResource(
                                cpuDescriptorSetRef.images[bindingRef.resourceIndex].desc.resource.handle)) {
                            // expecting planar formats and making sure that there is enough descriptors
                            constexpr uint32_t descriptorCountMultiplier = 3u;
                            descriptorCount *= descriptorCountMultiplier;
                        }
                        descriptorCounts[descTypeIndex] += descriptorCount;
                    }
                }
            }
        }
        if (descriptorSetCount > 0) {
            auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
            PLUGIN_ASSERT(descriptorPool.additionalPlatformDescriptorPool == VK_NULL_HANDLE);
            descriptorPoolSizes_.clear();
            descriptorPoolSizes_.reserve(OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE);
            // no buffering, only descriptors for one frame
            for (uint32_t idx = 0; idx < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE; ++idx) {
                const uint32_t count = descriptorCounts[idx];
                if (count > 0) {
                    descriptorPoolSizes_.push_back(VkDescriptorPoolSize { GetDescriptorTypeVk(idx), count });
                }
            }
            if (!descriptorPoolSizes_.empty()) {
                descriptorPool.additionalPlatformDescriptorPool =
                    CreatePoolFunc(vkDevice_, descriptorSetCount, descriptorPoolSizes_);
            }
        }
    }
    // create one frame descriptor pool
    {
        auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
        auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];

        PLUGIN_ASSERT(descriptorPool.descriptorPool == VK_NULL_HANDLE);
        const auto descriptorSetCount = static_cast<uint32_t>(cpuDescriptorSets.size());
        if (descriptorSetCount > 0) {
            descriptorPoolSizes_.clear();
            descriptorPoolSizes_.reserve(OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE);
            for (uint32_t idx = 0; idx < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE; ++idx) {
                const uint32_t count = oneFrameDescriptorNeed_.descriptorCount[idx];
                if (count > 0) {
                    descriptorPoolSizes_.push_back(VkDescriptorPoolSize { GetDescriptorTypeVk(idx), count });
                }
            }

            if (!descriptorPoolSizes_.empty()) {
                descriptorPool.descriptorPool = CreatePoolFunc(vkDevice_, descriptorSetCount, descriptorPoolSizes_);
            }
        }
        // check the need for additional platform conversion bindings
        if (hasPlatformConversionBindings_) {
            uint32_t platConvDescriptorCounts[OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE] { 0 };
            for (const auto& cpuDescriptorSetRef : cpuDescriptorSets) {
                if (cpuDescriptorSetRef.hasPlatformConversionBindings) {
                    for (const auto& bindingRef : cpuDescriptorSetRef.bindings) {
                        uint32_t descriptorCount = bindingRef.binding.descriptorCount;
                        const auto descTypeIndex = GetDescriptorIndex(bindingRef.binding.descriptorType);
                        if (descTypeIndex < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE) {
                            if ((bindingRef.binding.descriptorType ==
                                    DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                                RenderHandleUtil::IsPlatformConversionResource(
                                    cpuDescriptorSetRef.images[bindingRef.resourceIndex].desc.resource.handle)) {
                                // expecting planar formats and making sure that there is enough descriptors
                                constexpr uint32_t descriptorCountMultiplier = 3u;
                                descriptorCount *= descriptorCountMultiplier;
                            }
                            platConvDescriptorCounts[descTypeIndex] += descriptorCount;
                        }
                    }
                }
            }
            if (descriptorSetCount > 0) {
                PLUGIN_ASSERT(descriptorPool.additionalPlatformDescriptorPool == VK_NULL_HANDLE);
                descriptorPoolSizes_.clear();
                descriptorPoolSizes_.reserve(OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE);
                // no buffering, only descriptors for one frame
                for (uint32_t idx = 0; idx < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE; ++idx) {
                    const uint32_t count = platConvDescriptorCounts[idx];
                    if (count > 0) {
                        descriptorPoolSizes_.push_back(VkDescriptorPoolSize { GetDescriptorTypeVk(idx), count });
                    }
                }
                if (!descriptorPoolSizes_.empty()) {
                    descriptorPool.additionalPlatformDescriptorPool =
                        CreatePoolFunc(vkDevice_, descriptorSetCount, descriptorPoolSizes_);
                }
            }
        }
    }
}

RenderHandle NodeContextDescriptorSetManagerVk::CreateDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSets.size() >= maxSets_) {
        PLUGIN_LOG_E("RENDER_VALIDATION: No more descriptor sets available");
    }
#endif
    if (cpuDescriptorSets.size() < maxSets_) {
        CpuDescriptorSet newSet;
        LowLevelContextDescriptorPoolVk::DescriptorSetData descSetData;
        CreateCpuDescriptorSetData(descriptorSetLayoutBindings, false, newSet, descSetData, nullptr);

        const auto arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
        cpuDescriptorSets.push_back(move(newSet));
        // allocate storage from vector to gpu descriptor sets
        // don't create the actual gpu descriptor sets yet
        descriptorPool.descriptorSets.push_back(descSetData);

        // NOTE: can be used directly to index
        clientHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, arrayIndex, 0);
    }

    return clientHandle;
}

RenderHandle NodeContextDescriptorSetManagerVk::CreateOneFrameDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    CpuDescriptorSet newSet;
    LowLevelContextDescriptorPoolVk::DescriptorSetData descSetData;

    CreateCpuDescriptorSetData(descriptorSetLayoutBindings, true, newSet, descSetData, &oneFrameDescriptorNeed_);

    const auto arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
    cpuDescriptorSets.push_back(move(newSet));
    // allocate storage from vector to gpu descriptor sets
    // don't create the actual gpu descriptor sets yet
    descriptorPool.descriptorSets.push_back(descSetData);

    // NOTE: can be used directly to index
    clientHandle = RenderHandleUtil::CreateHandle(
        RenderHandleType::DESCRIPTOR_SET, arrayIndex, oneFrameDescSetGeneration_, ONE_FRAME_DESC_SET_BIT);

    return clientHandle;
}

const LowLevelDescriptorSetVk* NodeContextDescriptorSetManagerVk::GetDescriptorSet(const RenderHandle handle) const
{
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return ((const DescriptorSetManagerVk&)globalDescriptorSetMgr_).GetDescriptorSet(handle);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);

        const auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
        const auto& descriptorPool = descriptorPool_[descSetIdx];
        const LowLevelDescriptorSetVk* set = nullptr;
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        if (arrayIndex < (uint32_t)cpuDescriptorSets.size()) {
            if (arrayIndex < descriptorPool.descriptorSets.size()) {
                set = GetDescriptorSetFunc(
                    cpuDescriptorSets[arrayIndex], descriptorPool.descriptorSets[arrayIndex], debugName_, handle);
            }

            return set;
        }
    }
    return nullptr;
}

LowLevelContextDescriptorWriteDataVk& NodeContextDescriptorSetManagerVk::GetLowLevelDescriptorWriteData()
{
    return lowLevelDescriptorWriteData_;
}

bool NodeContextDescriptorSetManagerVk::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    PLUGIN_ASSERT(descSetIdx != ~0U);
    PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
    auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    auto& descriptorPool = descriptorPool_[descSetIdx];
    const uint32_t bufferingCount = (descSetIdx == DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME) ? 1u : bufferingCount_;
    bool retValue = false;
    if (arrayIndex < (uint32_t)cpuDescriptorSets.size()) {
        CpuDescriptorSet& refCpuSet = cpuDescriptorSets[arrayIndex];

        // with platform buffer bindings descriptor set creation needs to be checked
        if (refCpuSet.hasPlatformConversionBindings) {
            // no buffering
            CreateGpuDescriptorSetFunc(device_, 1u, handle, refCpuSet, descriptorPool, debugName_);
        } else if (!refCpuSet.gpuDescriptorSetCreated) { // deferred creation
            CreateGpuDescriptorSetFunc(device_, bufferingCount, handle, refCpuSet, descriptorPool, debugName_);
            refCpuSet.gpuDescriptorSetCreated = true;
        }

        // at the moment should be always dirty when coming here from the backend update
        if (refCpuSet.isDirty) {
            refCpuSet.isDirty = false;
            // advance to next gpu descriptor set
            if (descSetIdx != DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME) {
                refCpuSet.currentGpuBufferingIndex = (refCpuSet.currentGpuBufferingIndex + 1) % bufferingCount_;
            }
            retValue = true;
        }
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("invalid handle in descriptor set management");
#endif
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (descSetIdx == DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME) {
        const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (generationIndex != oneFrameDescSetGeneration_) {
            PLUGIN_LOG_E("RENDER_VALIDATION: invalid one frame descriptor set handle generation. One frame "
                         "descriptor sets "
                         "can only be used once.");
        }
    }
#endif
    return retValue;
}

void NodeContextDescriptorSetManagerVk::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    lowLevelDescriptorWriteData_.writeBindingCount += static_cast<uint32_t>(bindingResources.bindings.size());

    lowLevelDescriptorWriteData_.bufferBindingCount += static_cast<uint32_t>(bindingResources.buffers.size());
    lowLevelDescriptorWriteData_.imageBindingCount += static_cast<uint32_t>(bindingResources.images.size());
    lowLevelDescriptorWriteData_.samplerBindingCount += static_cast<uint32_t>(bindingResources.samplers.size());
}

void NodeContextDescriptorSetManagerVk::ClearDescriptorSetWriteData()
{
    lowLevelDescriptorWriteData_.Clear();
}

void NodeContextDescriptorSetManagerVk::ResizeDescriptorSetWriteData()
{
    auto& descWd = lowLevelDescriptorWriteData_;
    if (descWd.writeBindingCount > 0U) {
        lowLevelDescriptorWriteData_.writeDescriptorSets.resize(descWd.writeBindingCount);
        lowLevelDescriptorWriteData_.descriptorBufferInfos.resize(descWd.bufferBindingCount);
        lowLevelDescriptorWriteData_.descriptorImageInfos.resize(descWd.imageBindingCount);
        lowLevelDescriptorWriteData_.descriptorSamplerInfos.resize(descWd.samplerBindingCount);
#if (RENDER_VULKAN_RT_ENABLED == 1)
        lowLevelDescriptorWriteData_.descriptorAccelInfos.resize(descWd.bufferBindingCount);
#endif
    }
}

RENDER_END_NAMESPACE()
