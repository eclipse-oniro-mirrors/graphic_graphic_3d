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

#include <vulkan/vulkan.h>

#include <base/util/formats.h>

#include "vulkan/device_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

struct OH_NativeBuffer;

RENDER_BEGIN_NAMESPACE()
namespace PlatformHardwareBufferUtil {
HardwareBufferProperties QueryHwBufferFormatProperties(const DeviceVk& deviceVk, uintptr_t hwBuffer)
{
    HardwareBufferProperties hardwareBufferProperties;

    const DevicePlatformDataVk& devicePlat = ((const DevicePlatformDataVk&)deviceVk.GetPlatformData());
    const VkDevice device = devicePlat.device;
    const PlatformExtFunctions& extFunctions = deviceVk.GetPlatformExtFunctions();

    OH_NativeBuffer* nativeBuffer = static_cast<OH_NativeBuffer*>(reinterpret_cast<void*>(hwBuffer));
    if (nativeBuffer && extFunctions.vkGetNativeBufferPropertiesOHOS && extFunctions.vkGetMemoryNativeBufferOHOS) {
        VkNativeBufferFormatPropertiesOHOS bufferFormatProperties {};
        bufferFormatProperties.sType = VK_STRUCTURE_TYPE_NATIVE_BUFFER_FORMAT_PROPERTIES_OHOS;

        VkNativeBufferPropertiesOHOS bufferProperties {};
        bufferProperties.sType = VK_STRUCTURE_TYPE_NATIVE_BUFFER_PROPERTIES_OHOS;
        bufferProperties.pNext = &bufferFormatProperties;

        VALIDATE_VK_RESULT(extFunctions.vkGetNativeBufferPropertiesOHOS(device, // device
            nativeBuffer,                                                       // buffer
            &bufferProperties));                                                // pProperties

        PLUGIN_ASSERT_MSG(bufferProperties.allocationSize > 0, "ohos native buffer allocation size is zero");
        PLUGIN_ASSERT_MSG(bufferFormatProperties.externalFormat != 0, "ohos native buffer externalFormat cannot be 0");

        hardwareBufferProperties.allocationSize = bufferProperties.allocationSize;
        hardwareBufferProperties.memoryTypeBits = bufferProperties.memoryTypeBits;

        hardwareBufferProperties.format = bufferFormatProperties.format;
        hardwareBufferProperties.externalFormat = bufferFormatProperties.externalFormat;
        hardwareBufferProperties.formatFeatures = bufferFormatProperties.formatFeatures;
        hardwareBufferProperties.samplerYcbcrConversionComponents =
            bufferFormatProperties.samplerYcbcrConversionComponents;
        hardwareBufferProperties.suggestedYcbcrModel = bufferFormatProperties.suggestedYcbcrModel;
        hardwareBufferProperties.suggestedYcbcrRange = bufferFormatProperties.suggestedYcbcrRange;
        hardwareBufferProperties.suggestedXChromaOffset = bufferFormatProperties.suggestedXChromaOffset;
        hardwareBufferProperties.suggestedYChromaOffset = bufferFormatProperties.suggestedYChromaOffset;
    }

    return hardwareBufferProperties;
}

HardwareBufferImage CreateHwPlatformImage(const DeviceVk& deviceVk, const HardwareBufferProperties& hwBufferProperties,
    const GpuImageDesc& desc, uintptr_t hwBuffer)
{
    HardwareBufferImage hwBufferImage;
    GpuImageDesc validDesc = desc;
    const bool useExternalFormat = hwBufferProperties.externalFormat != 0U;
    if (useExternalFormat) {
        validDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT;
    }
    VkImageCreateInfo imageCreateInfo = GetHwBufferImageCreateInfo(validDesc);

    VkExternalFormatOHOS externalFormat {
        VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_OHOS, // sType
        nullptr,                                // pNext
        hwBufferProperties.externalFormat,      // externalFormat
    };
    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo {
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,        // sType
        nullptr,                                                    // pNext
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OHOS_NATIVE_BUFFER_BIT_OHOS, // handleTypes
    };
    imageCreateInfo.pNext = &externalMemoryImageCreateInfo;
    if (useExternalFormat) { // chain external format
        externalMemoryImageCreateInfo.pNext = &externalFormat;
    }

    const DevicePlatformDataVk& platData = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkDevice device = platData.device;
    VALIDATE_VK_RESULT(vkCreateImage(device, // device
        &imageCreateInfo,                    // pCreateInfo
        nullptr,                             // pAllocator
        &hwBufferImage.image));              // pImage

    // some older version of validation layers required calling vkGetImageMemoryRequirements before
    // vkAllocateMemory. with at least 1.3.224.1 it's an error to call vkGetImageMemoryRequirements:
    // "If image was created with the VK_EXTERNAL_MEMORY_HANDLE_TYPE_OHOS_NATIVE_BUFFER_BIT_OPENHARMONY external memory
    // handle type, then image must be bound to memory."

    // get memory type index based on
    const uint32_t memoryTypeIndex =
        GetMemoryTypeIndex(platData.physicalDeviceProperties.physicalDeviceMemoryProperties,
            hwBufferProperties.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo memoryAllocateInfo {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
        nullptr,                                // pNext
        hwBufferProperties.allocationSize,      // allocationSize
        memoryTypeIndex,                        // memoryTypeIndex
    };

    OH_NativeBuffer* nativeBuffer = static_cast<OH_NativeBuffer*>(reinterpret_cast<void*>(hwBuffer));
    PLUGIN_ASSERT(nativeBuffer);
    VkMemoryDedicatedAllocateInfo dedicatedMemoryAllocateInfo {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, // sType;
        nullptr,                                          // pNext
        hwBufferImage.image,                              // image
        VK_NULL_HANDLE,                                   // buffer
    };
    VkImportNativeBufferInfoOHOS importHardwareBufferInfo {
        VK_STRUCTURE_TYPE_IMPORT_NATIVE_BUFFER_INFO_OHOS, // sType
        &dedicatedMemoryAllocateInfo,                     // pNext
        nativeBuffer,                                     // buffer
    };
    memoryAllocateInfo.pNext = &importHardwareBufferInfo;

    VALIDATE_VK_RESULT(vkAllocateMemory(device,  // device
        &memoryAllocateInfo,                     // pAllocateInfo
        nullptr,                                 // pAllocator
        &hwBufferImage.deviceMemory));           // pMemory
    VALIDATE_VK_RESULT(vkBindImageMemory(device, // device
        hwBufferImage.image,                     // image
        hwBufferImage.deviceMemory,              // memory
        0));                                     // memoryOffset

    return hwBufferImage;
}

HardwareBufferBuffer CreateHwPlatformBuffer(const DeviceVk& deviceVk,
    const HardwareBufferProperties& hwBufferProperties, const GpuBufferDesc& desc, uintptr_t hwBuffer)
{
    constexpr VkBufferCreateFlags bufferCreateFlags { 0 };

    VkBufferCreateInfo bufferCreateInfo {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,     // sType
        nullptr,                                  // pNext
        bufferCreateFlags,                        // flags
        hwBufferProperties.allocationSize,        // size
        desc.usageFlags,                          // usage
        VkSharingMode::VK_SHARING_MODE_EXCLUSIVE, // sharingMode
        0,                                        // queueFamilyIndexCount
        nullptr,                                  // pQueueFamilyIndices
    };
    VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo {
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO,               // sType
        nullptr,                                                            // pNext
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID, // handleTypes
    };
    bufferCreateInfo.pNext = &externalMemoryBufferCreateInfo;

    const auto& platData = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkDevice device = platData.device;
    HardwareBufferBuffer hwBufferBuffer { VK_NULL_HANDLE, VK_NULL_HANDLE };
    VALIDATE_VK_RESULT(vkCreateBuffer(device, // device
        &bufferCreateInfo,                    // pCreateInfo
        nullptr,                              // pAllocator
        &hwBufferBuffer.buffer));             // pImage
    // get memory type index based on
    const uint32_t memoryTypeIndex =
        GetMemoryTypeIndex(platData.physicalDeviceProperties.physicalDeviceMemoryProperties,
            hwBufferProperties.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo memoryAllocateInfo {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
        nullptr,                                // pNext
        hwBufferProperties.allocationSize,      // allocationSize
        memoryTypeIndex,                        // memoryTypeIndex
    };

    OH_NativeBuffer* nativeBuffer = static_cast<OH_NativeBuffer*>(reinterpret_cast<void*>(hwBuffer));
    PLUGIN_ASSERT(nativeBuffer);

    VkImportNativeBufferInfoOHOS importHardwareBufferInfo {
        VK_STRUCTURE_TYPE_IMPORT_NATIVE_BUFFER_INFO_OHOS, // sType
        nullptr,                                          // pNext
        nativeBuffer,                                     // buffer
    };
    memoryAllocateInfo.pNext = &importHardwareBufferInfo;

    VALIDATE_VK_RESULT(vkAllocateMemory(device,   // device
        &memoryAllocateInfo,                      // pAllocateInfo
        nullptr,                                  // pAllocator
        &hwBufferBuffer.deviceMemory));           // pMemory
    VALIDATE_VK_RESULT(vkBindBufferMemory(device, // device
        hwBufferBuffer.buffer,                    // image
        hwBufferBuffer.deviceMemory,              // memory
        0));                                      // memoryOffset
    return hwBufferBuffer;
}
} // namespace PlatformHardwareBufferUtil
RENDER_END_NAMESPACE()
