/*
 * Copyright (c) 2021 The Khronos Group Inc.
 * Copyright (c) 2021 Valve Corporation
 * Copyright (c) 2021 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included in
 * all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * Author: Charles Giessen <charles@lunarg.com>
 */

#pragma once

#include "common.h"
#include "vulkan_library_path.h"

struct VulkanFunctions {
    LibraryWrapper loader;

    // Pre-Instance
    PFN_vkGetInstanceProcAddr fp_vkGetInstanceProcAddr = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties fp_vkEnumerateInstanceExtensionProperties = nullptr;
    PFN_vkEnumerateInstanceLayerProperties fp_vkEnumerateInstanceLayerProperties = nullptr;
    PFN_vkEnumerateInstanceVersion fp_vkEnumerateInstanceVersion = nullptr;
    PFN_vkCreateInstance fp_vkCreateInstance = nullptr;

    // Instance
    PFN_vkDestroyInstance fp_vkDestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices fp_vkEnumeratePhysicalDevices = nullptr;
    PFN_vkGetPhysicalDeviceFeatures fp_vkGetPhysicalDeviceFeatures = nullptr;
    PFN_vkGetPhysicalDeviceFeatures2 fp_vkGetPhysicalDeviceFeatures2 = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties fp_vkGetPhysicalDeviceFormatProperties = nullptr;
    PFN_vkGetPhysicalDeviceImageFormatProperties fp_vkGetPhysicalDeviceImageFormatProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties fp_vkGetPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties2 fp_vkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties fp_vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 fp_vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties fp_vkGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties2 fp_vkGetPhysicalDeviceFormatProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties2 fp_vkGetPhysicalDeviceMemoryProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fp_vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fp_vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fp_vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties fp_vkEnumerateDeviceExtensionProperties = nullptr;
    PFN_vkEnumerateDeviceLayerProperties fp_vkEnumerateDeviceLayerProperties = nullptr;

    PFN_vkGetDeviceProcAddr fp_vkGetDeviceProcAddr = nullptr;
    PFN_vkCreateDevice fp_vkCreateDevice = nullptr;

// WSI
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkCreateAndroidSurfaceKHR fp_vkCreateAndroidSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    PFN_vkCreateMetalSurfaceEXT fp_vkCreateMetalSurfaceEXT
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        PFN_vkCreateWaylandSurfaceKHR fp_vkCreateWaylandSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR fp_vkCreateXcbSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR fp_vkCreateXlibSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR fp_vkCreateWin32SurfaceKHR;
#endif
    PFN_vkDestroySurfaceKHR fp_vkDestroySurfaceKHR = nullptr;

    // device functions
    PFN_vkDestroyDevice fp_vkDestroyDevice = nullptr;
    PFN_vkGetDeviceQueue fp_vkGetDeviceQueue = nullptr;

    VulkanFunctions();
};

struct InstanceCreateInfo {
    VkInstanceCreateInfo inst_info{};
    VkApplicationInfo app_info{};
    std::string app_name;
    std::string engine_name;
    uint32_t app_version = 0;
    uint32_t engine_version = 0;
    uint32_t api_version = VK_MAKE_VERSION(1, 0, 0);
    std::vector<const char*> enabled_layers;
    std::vector<const char*> enabled_extensions;

    InstanceCreateInfo();

    VkInstanceCreateInfo* get() noexcept;
    InstanceCreateInfo& set_application_name(std::string app_name);
    InstanceCreateInfo& set_engine_name(std::string engine_name);
    InstanceCreateInfo& set_app_version(uint32_t app_version);
    InstanceCreateInfo& set_engine_version(uint32_t engine_version);
    InstanceCreateInfo& set_api_version(uint32_t api_version);
    InstanceCreateInfo& add_layer(const char* layer_name);
    InstanceCreateInfo& add_extension(const char* ext_name);
};

struct DeviceQueueCreateInfo {
    VkDeviceQueueCreateInfo queue{};
    std::vector<float> priorities;
    DeviceQueueCreateInfo();
    DeviceQueueCreateInfo& add_priority(float priority);
    DeviceQueueCreateInfo& set_props(VkQueueFamilyProperties props);
};

struct DeviceCreateInfo {
    VkDeviceCreateInfo dev{};
    std::vector<const char*> enabled_extensions;
    std::vector<const char*> enabled_layers;
    std::vector<DeviceQueueCreateInfo> queue_info_details;
    std::vector<VkDeviceQueueCreateInfo> queue_infos;

    VkDeviceCreateInfo* get() noexcept;
    DeviceCreateInfo& add_layer(const char* layer_name);
    DeviceCreateInfo& add_extension(const char* ext_name);
    DeviceCreateInfo& add_device_queue(DeviceQueueCreateInfo queue_info_detail);
};

struct InstWrapper {
    InstWrapper(VulkanFunctions& functions) noexcept : functions(&functions) {}
    InstWrapper(VulkanFunctions& functions, VkInstance inst) noexcept : functions(&functions), inst(inst) {}
    ~InstWrapper() {
        if (inst != VK_NULL_HANDLE) functions->fp_vkDestroyInstance(inst, nullptr);
    }

    // Immoveable object
    InstWrapper(InstWrapper const&) = delete;
    InstWrapper& operator=(InstWrapper const&) = delete;
    InstWrapper(InstWrapper&&) = delete;
    InstWrapper& operator=(InstWrapper&&) = delete;

    // Convenience
    operator VkInstance() { return inst; }
    VulkanFunctions* operator->() { return functions; }

    VulkanFunctions* functions = nullptr;
    VkInstance inst = VK_NULL_HANDLE;
};

VkResult CreateInst(InstWrapper& inst, InstanceCreateInfo& inst_info);
VkResult CreatePhysDevs(InstWrapper& inst, uint32_t phys_dev_count, std::vector<VkPhysicalDevice>& physical_devices);
VkResult CreatePhysDev(InstWrapper& inst, VkPhysicalDevice& physical_device);

struct DeviceWrapper {
    DeviceWrapper(){};
    DeviceWrapper(VulkanFunctions& functions, VkDevice dev) : functions(&functions), dev(dev){};
    ~DeviceWrapper() { functions->fp_vkDestroyDevice(dev, nullptr); }

    // Immoveable object
    DeviceWrapper(DeviceWrapper const&) = delete;
    DeviceWrapper& operator=(DeviceWrapper const&) = delete;
    DeviceWrapper(DeviceWrapper&&) = delete;
    DeviceWrapper& operator=(DeviceWrapper&&) = delete;

    // Convenience
    operator VkDevice() { return dev; }
    VulkanFunctions* operator->() { return functions; }

    VulkanFunctions* functions = nullptr;
    VkDevice dev = VK_NULL_HANDLE;
};

inline bool operator==(const VkExtent3D & a, const VkExtent3D & b) {
    return a.width == b.width && a.height == b.height && a.depth == b.depth;
}

inline bool operator==(const VkQueueFamilyProperties& a, const VkQueueFamilyProperties& b) {
    return a.minImageTransferGranularity == b.minImageTransferGranularity && a.queueCount == b.queueCount &&
           a.queueFlags == b.queueFlags && a.timestampValidBits == b.timestampValidBits;
}