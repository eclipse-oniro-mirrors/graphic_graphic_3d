/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gpu_buffer_vk.h"

#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include <base/math/mathf.h>

#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif

#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_buffer.h"
#include "device/gpu_resource_desc_flag_validation.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_memory_allocator_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t GetAlignedByteSize(const uint32_t byteSize, const uint32_t alignment)
{
    return (byteSize + alignment - 1) & (~(alignment - 1));
}

constexpr uint32_t GetMinBufferAlignment(const VkPhysicalDeviceLimits& limits)
{
    return Math::max(static_cast<uint32_t>(limits.minStorageBufferOffsetAlignment),
        static_cast<uint32_t>(limits.minUniformBufferOffsetAlignment));
}

constexpr uint32_t GetMemoryMapAlignment(const VkPhysicalDeviceLimits& limits)
{
    return Math::max(
        static_cast<uint32_t>(limits.minMemoryMapAlignment), static_cast<uint32_t>(limits.nonCoherentAtomSize));
}

GpuResourceMemoryVk GetPlatMemory(const VmaAllocationInfo& allocationInfo, const VkMemoryPropertyFlags flags)
{
    return GpuResourceMemoryVk {
        allocationInfo.deviceMemory,
        allocationInfo.offset,
        allocationInfo.size,
        allocationInfo.pMappedData,
        allocationInfo.memoryType,
        flags,
    };
}

#if (RENDER_PERF_ENABLED == 1)
void RecordAllocation(
    PlatformGpuMemoryAllocator& gpuMemAllocator, const GpuBufferDesc& desc, const int64_t alignedByteSize)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        CORE_NS::IPerformanceDataManager* pdm = inst->Get("Memory");

        pdm->UpdateData("AllGpuBuffers", "GPU_BUFFER", alignedByteSize);
        const string poolDebugName = gpuMemAllocator.GetBufferPoolDebugName(desc);
        if (!poolDebugName.empty()) {
            pdm->UpdateData(poolDebugName, "GPU_BUFFER", alignedByteSize);
        }
    }
}
#endif
} // namespace

GpuBufferVk::GpuBufferVk(Device& device, const GpuBufferDesc& desc)
    : device_(device), desc_(desc),
      isPersistantlyMapped_(
          (desc.memoryPropertyFlags & MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
          (desc.memoryPropertyFlags & MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)),
      isRingBuffer_(desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER),
      bufferingCount_(isRingBuffer_ ? device_.GetCommandBufferingCount() : 1u)
{
    PLUGIN_ASSERT_MSG(
        (isRingBuffer_ && isPersistantlyMapped_) || !isRingBuffer_, "dynamic ring buffer needs persistend mapping");

    VkMemoryPropertyFlags memoryPropertyFlags = static_cast<VkMemoryPropertyFlags>(desc_.memoryPropertyFlags);
    const VkMemoryPropertyFlags requiredFlags =
        (memoryPropertyFlags & (~(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
                                   CORE_MEMORY_PROPERTY_PROTECTED_BIT)));
    const VkMemoryPropertyFlags preferredFlags = memoryPropertyFlags;

    const auto& limits = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData())
                             .physicalDeviceProperties.physicalDeviceProperties.limits;
    // force min buffer alignment always
    const uint32_t minBufferAlignment = GetMinBufferAlignment(limits);
    const uint32_t minMapAlignment = (isRingBuffer_ || isPersistantlyMapped_) ? GetMemoryMapAlignment(limits) : 1u;
    plat_.bindMemoryByteSize = GetAlignedByteSize(desc_.byteSize, Math::max(minBufferAlignment, minMapAlignment));
    plat_.fullByteSize = plat_.bindMemoryByteSize * bufferingCount_;
    plat_.currentByteOffset = 0;
    plat_.usage = static_cast<VkBufferUsageFlags>(desc_.usageFlags);

    AllocateMemory(requiredFlags, preferredFlags);

    if (PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator(); gpuMemAllocator) {
        const VkMemoryPropertyFlags memFlags =
            (VkMemoryPropertyFlags)gpuMemAllocator->GetMemoryTypeProperties(mem_.allocationInfo.memoryType);
        isMappable_ = (memFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? true : false;
#if (RENDER_PERF_ENABLED == 1)
        RecordAllocation(*gpuMemAllocator, desc, plat_.fullByteSize);
#endif
    }

#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu buffer id >: 0x%" PRIxPTR, (uintptr_t)plat_.buffer);
#endif
}

GpuBufferVk::~GpuBufferVk()
{
    if (isMapped_) {
        Unmap();
    }

    if (PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator(); gpuMemAllocator) {
        gpuMemAllocator->DestroyBuffer(plat_.buffer, mem_.allocation);
#if (RENDER_PERF_ENABLED == 1)
        RecordAllocation(*gpuMemAllocator, desc_, -static_cast<int64_t>(plat_.fullByteSize));
#endif
    }
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu buffer id <: 0x%" PRIxPTR, (uintptr_t)plat_.buffer);
#endif
}

const GpuBufferDesc& GpuBufferVk::GetDesc() const
{
    return desc_;
}

const GpuBufferPlatformDataVk& GpuBufferVk::GetPlatformData() const
{
    return plat_;
}

void* GpuBufferVk::Map()
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to map non-mappable gpu buffer");
        return nullptr;
    }
    if (isMapped_) {
        PLUGIN_LOG_E("gpu buffer already mapped");
        Unmap();
    }
    isMapped_ = true;

    if (isRingBuffer_) {
        plat_.currentByteOffset = (plat_.currentByteOffset + plat_.bindMemoryByteSize) % plat_.fullByteSize;
    }

    void* data { nullptr };
    if (isPersistantlyMapped_) {
        data = reinterpret_cast<uint8_t*>(mem_.allocationInfo.pMappedData) + plat_.currentByteOffset;
    } else {
        PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
        if (gpuMemAllocator) {
            data = gpuMemAllocator->MapMemory(mem_.allocation);
        }
    }
    return data;
}

void* GpuBufferVk::MapMemory()
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to map non-mappable gpu buffer");
        return nullptr;
    }
    if (isMapped_) {
        PLUGIN_LOG_E("gpu buffer already mapped");
        Unmap();
    }
    isMapped_ = true;

    void* data { nullptr };
    if (isPersistantlyMapped_) {
        data = mem_.allocationInfo.pMappedData;
    } else {
        PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
        if (gpuMemAllocator) {
            data = gpuMemAllocator->MapMemory(mem_.allocation);
        }
    }
    return data;
}

void GpuBufferVk::Unmap() const
{
    if (!isMappable_) {
        PLUGIN_LOG_E("trying to unmap non-mappable gpu buffer");
    }
    if (!isMapped_) {
        PLUGIN_LOG_E("gpu buffer not mapped");
    }
    isMapped_ = false;

    if (!isPersistantlyMapped_) {
        PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
        if (gpuMemAllocator) {
            gpuMemAllocator->FlushAllocation(mem_.allocation, 0, VK_WHOLE_SIZE);
            gpuMemAllocator->UnmapMemory(mem_.allocation);
        }
    }
}

void GpuBufferVk::AllocateMemory(const VkMemoryPropertyFlags requiredFlags, const VkMemoryPropertyFlags preferredFlags)
{
    constexpr VkBufferCreateFlags bufferCreateFlags { 0 };
    const VkBufferCreateInfo bufferCreateInfo {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,     // sType
        nullptr,                                  // pNext
        bufferCreateFlags,                        // flags
        (VkDeviceSize)plat_.fullByteSize,         // size
        plat_.usage,                              // usage
        VkSharingMode::VK_SHARING_MODE_EXCLUSIVE, // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
    };

    VmaAllocationCreateFlags allocationCreateFlags { 0 };
    if (isPersistantlyMapped_) {
        allocationCreateFlags |=
            (VmaAllocationCreateFlags)(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                       VmaAllocationCreateFlagBits::
                                           VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
    PLUGIN_ASSERT(gpuMemAllocator);
    if (gpuMemAllocator) {
        // can be null handle -> default allocator
        const VmaPool customPool = gpuMemAllocator->GetBufferPool(desc_);
        const VmaAllocationCreateInfo allocationCreateInfo {
            allocationCreateFlags,                 // flags
            VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO, // usage
            requiredFlags,                         // requiredFlags
            preferredFlags,                        // preferredFlags
            0,                                     // memoryTypeBits
            customPool,                            // pool
            nullptr,                               // pUserData
            0.f,                                   // priority
        };

        gpuMemAllocator->CreateBuffer(
            bufferCreateInfo, allocationCreateInfo, plat_.buffer, mem_.allocation, mem_.allocationInfo);
    }

    plat_.memory = GetPlatMemory(mem_.allocationInfo, preferredFlags);
}

RENDER_END_NAMESPACE()
