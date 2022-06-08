/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_CREATE_FUNCTIONS_VK_H
#define VULKAN_CREATE_FUNCTIONS_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

RENDER_BEGIN_NAMESPACE()
struct LowLevelQueueInfo;
struct VersionInfo;

struct InstanceWrapper {
    VkInstance instance { VK_NULL_HANDLE };
    bool debugReportSupported { false };
    bool debugUtilsSupported { false };
    uint32_t apiMajor { 0u };
    uint32_t apiMinor { 0u };
};

struct PhysicalDeviceWrapper {
    VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
    BASE_NS::vector<VkExtensionProperties> physicalDeviceExtensions;
    PhysicalDevicePropertiesVk physicalDeviceProperties;
};

struct DeviceWrapper {
    VkDevice device { VK_NULL_HANDLE };
    BASE_NS::vector<BASE_NS::string> extensions;
};

struct QueueProperties {
    VkQueueFlags requiredFlags { 0 };
    uint32_t count { 0 };
    float priority { 1.0f };
    bool explicitFlags { false };
    bool canPresent { false };
};

struct Window {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    uintptr_t hinstance;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    uintptr_t connection;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    uintptr_t display;
#endif
    uintptr_t window;
};

class CreateFunctionsVk {
public:
    static InstanceWrapper CreateInstance(const VersionInfo& engineInfo, const VersionInfo& appInfo);
    static void DestroyInstance(VkInstance instance);

    static VkDebugReportCallbackEXT CreateDebugCallback(
        VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunction);
    static void DestroyDebugCallback(VkInstance instance, VkDebugReportCallbackEXT debugReport);

    static VkDebugUtilsMessengerEXT CreateDebugMessenger(
        VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunction);
    static void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);

    static PhysicalDeviceWrapper CreatePhysicalDevice(VkInstance instance, QueueProperties const& queueProperties);

    static DeviceWrapper CreateDevice(VkInstance instance, VkPhysicalDevice physicalDevice,
        const BASE_NS::vector<VkExtensionProperties>& physicalDeviceExtensions,
        const VkPhysicalDeviceFeatures& featuresToEnable, const VkPhysicalDeviceFeatures2* physicalDeviceFeatures2,
        const BASE_NS::vector<LowLevelQueueInfo>& availableQueues,
        const BASE_NS::vector<BASE_NS::string_view>& preferredDeviceExtensions);
    static void DestroyDevice(VkDevice device);

    static VkSurfaceKHR CreateSurface(VkInstance instance, Window const& nativeWindow);
    static void DestroySurface(VkInstance instance, VkSurfaceKHR surface);

    static void DestroySwapchain(VkDevice device, VkSwapchainKHR swapchain);

    static BASE_NS::vector<LowLevelQueueInfo> GetAvailableQueues(
        VkPhysicalDevice physicalDevice, const BASE_NS::vector<QueueProperties>& queueProperties);

    static VkPipelineCache CreatePipelineCache(VkDevice device, BASE_NS::array_view<const uint8_t> initialData);
    static void DestroyPipelineCache(VkDevice device, VkPipelineCache cache);
};
RENDER_END_NAMESPACE()

#endif // VULKAN_CREATE_FUNCTIONS_VK_H
