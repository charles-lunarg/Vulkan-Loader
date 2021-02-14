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

#include "test_util.h"

VulkanFunctions::VulkanFunctions() : loader(FRAMEWORK_VULKAN_LIBRARY_PATH) {
    // clang-format off
    fp_vkGetInstanceProcAddr = loader.get_symbol<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    fp_vkEnumerateInstanceExtensionProperties = loader.get_symbol<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
    fp_vkEnumerateInstanceLayerProperties = loader.get_symbol<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
    fp_vkEnumerateInstanceVersion = loader.get_symbol<PFN_vkEnumerateInstanceVersion>("vkEnumerateInstanceVersion");
    fp_vkCreateInstance = loader.get_symbol<PFN_vkCreateInstance>("vkCreateInstance");
    fp_vkDestroyInstance = loader.get_symbol<PFN_vkDestroyInstance>("vkDestroyInstance");
    fp_vkEnumeratePhysicalDevices = loader.get_symbol<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");
    fp_vkGetPhysicalDeviceFeatures = loader.get_symbol<PFN_vkGetPhysicalDeviceFeatures>("vkGetPhysicalDeviceFeatures");
    fp_vkGetPhysicalDeviceFeatures2 = loader.get_symbol<PFN_vkGetPhysicalDeviceFeatures2>("vkGetPhysicalDeviceFeatures2");
    fp_vkGetPhysicalDeviceFormatProperties = loader.get_symbol<PFN_vkGetPhysicalDeviceFormatProperties>("vkGetPhysicalDeviceFormatProperties");
    fp_vkGetPhysicalDeviceImageFormatProperties = loader.get_symbol<PFN_vkGetPhysicalDeviceImageFormatProperties>("vkGetPhysicalDeviceImageFormatProperties");
    fp_vkGetPhysicalDeviceProperties = loader.get_symbol<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties");
    fp_vkGetPhysicalDeviceProperties2 = loader.get_symbol<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
    fp_vkGetPhysicalDeviceQueueFamilyProperties = loader.get_symbol<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
    fp_vkGetPhysicalDeviceQueueFamilyProperties2 = loader.get_symbol<PFN_vkGetPhysicalDeviceQueueFamilyProperties2>("vkGetPhysicalDeviceQueueFamilyProperties2");
    fp_vkGetPhysicalDeviceMemoryProperties = loader.get_symbol<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties");
    fp_vkGetPhysicalDeviceFormatProperties2 = loader.get_symbol<PFN_vkGetPhysicalDeviceFormatProperties2>("vkGetPhysicalDeviceFormatProperties2");
    fp_vkGetPhysicalDeviceMemoryProperties2 = loader.get_symbol<PFN_vkGetPhysicalDeviceMemoryProperties2>("vkGetPhysicalDeviceMemoryProperties2");
    fp_vkGetPhysicalDeviceSurfaceSupportKHR = loader.get_symbol<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>("vkGetPhysicalDeviceSurfaceSupportKHR");
    fp_vkGetPhysicalDeviceSurfaceFormatsKHR = loader.get_symbol<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR");
    fp_vkGetPhysicalDeviceSurfacePresentModesKHR = loader.get_symbol<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>("vkGetPhysicalDeviceSurfacePresentModesKHR");
    fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = loader.get_symbol<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    fp_vkEnumerateDeviceExtensionProperties = loader.get_symbol<PFN_vkEnumerateDeviceExtensionProperties>("vkEnumerateDeviceExtensionProperties");
    fp_vkEnumerateDeviceLayerProperties = loader.get_symbol<PFN_vkEnumerateDeviceLayerProperties>("vkEnumerateDeviceLayerProperties");
    
    fp_vkDestroySurfaceKHR = loader.get_symbol<PFN_vkDestroySurfaceKHR>("vkDestroySurfaceKHR");
    fp_vkGetDeviceProcAddr = loader.get_symbol<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
    fp_vkCreateDevice = loader.get_symbol<PFN_vkCreateDevice>("vkCreateDevice");

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    fp_vkCreateAndroidSurfaceKHR = loader.get_symbol<PFN_vkCreateAndroidSurfaceKHR>("vkCreateAndroidSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    fp_vkCreateMetalSurfaceEXT = loader.get_symbol<PFN_vkCreateMetalSurfaceEXT>("vkCreateMetalSurfaceEXT")
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    fp_vkCreateWaylandSurfaceKHR = loader.get_symbol<PFN_vkCreateWaylandSurfaceKHR>("vkCreateWaylandSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    fp_vkCreateXcbSurfaceKHR = loader.get_symbol<PFN_vkCreateXcbSurfaceKHR>("vkCreateXcbSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    fp_vkCreateXlibSurfaceKHR = loader.get_symbol<PFN_vkCreateXlibSurfaceKHR>("vkCreateXlibSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    fp_vkCreateWin32SurfaceKHR = loader.get_symbol<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR");
#endif
    fp_vkDestroyDevice = loader.get_symbol<PFN_vkDestroyDevice>("vkDestroyDevice");
    fp_vkGetDeviceQueue = loader.get_symbol<PFN_vkGetDeviceQueue>("vkGetDeviceQueue");
    // clang-format on
}

InstanceCreateInfo::InstanceCreateInfo() {
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
}

VkInstanceCreateInfo* InstanceCreateInfo::get() noexcept {
    app_info.pApplicationName = app_name.c_str();
    app_info.pEngineName = engine_name.c_str();
    app_info.applicationVersion = app_version;
    app_info.engineVersion = engine_version;
    app_info.apiVersion = api_version;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
    inst_info.ppEnabledLayerNames = enabled_layers.data();
    inst_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    inst_info.ppEnabledExtensionNames = enabled_extensions.data();
    return &inst_info;
}
InstanceCreateInfo& InstanceCreateInfo::set_application_name(std::string app_name) {
    this->app_name = app_name;
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::set_engine_name(std::string engine_name) {
    this->engine_name = engine_name;
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::set_app_version(uint32_t app_version) {
    this->app_version = app_version;
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::set_engine_version(uint32_t engine_version) {
    this->engine_version = engine_version;
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::set_api_version(uint32_t api_version) {
    this->api_version = api_version;
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::add_layer(const char* layer_name) {
    enabled_layers.push_back(layer_name);
    return *this;
}
InstanceCreateInfo& InstanceCreateInfo::add_extension(const char* ext_name) {
    enabled_extensions.push_back(ext_name);
    return *this;
}

DeviceQueueCreateInfo::DeviceQueueCreateInfo() { queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; }
DeviceQueueCreateInfo& DeviceQueueCreateInfo::add_priority(float priority) {
    priorities.push_back(priority);
    return *this;
}
DeviceQueueCreateInfo& DeviceQueueCreateInfo::set_props(VkQueueFamilyProperties props) {
    queue.queueCount = props.queueCount;
    return *this;
}

VkDeviceCreateInfo* DeviceCreateInfo::get() noexcept {
    dev.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
    dev.ppEnabledLayerNames = enabled_layers.data();
    dev.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    dev.ppEnabledExtensionNames = enabled_extensions.data();
    uint32_t index = 0;
    for (auto& queue : queue_infos) queue.queueFamilyIndex = index++;

    dev.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    dev.pQueueCreateInfos = queue_infos.data();
    return &dev;
}
DeviceCreateInfo& DeviceCreateInfo::add_layer(const char* layer_name) {
    enabled_layers.push_back(layer_name);

    return *this;
}
DeviceCreateInfo& DeviceCreateInfo::add_extension(const char* ext_name) {
    enabled_extensions.push_back(ext_name);

    return *this;
}
DeviceCreateInfo& DeviceCreateInfo::add_device_queue(DeviceQueueCreateInfo queue_info_detail) {
    queue_info_details.push_back(queue_info_detail);
    return *this;
}

VkResult CreateInst(InstWrapper& inst, InstanceCreateInfo& inst_info) {
    return inst.functions->fp_vkCreateInstance(inst_info.get(), nullptr, &inst.inst);
}

VkResult CreatePhysDevs(InstWrapper& inst, uint32_t phys_dev_count, std::vector<VkPhysicalDevice>& physical_devices) {
    physical_devices.resize(phys_dev_count);
    uint32_t physical_count = phys_dev_count;
    return inst.functions->fp_vkEnumeratePhysicalDevices(inst.inst, &physical_count, physical_devices.data());
}

VkResult CreatePhysDev(InstWrapper& inst, VkPhysicalDevice& physical_device) {
    uint32_t physical_count = 1;
    return inst.functions->fp_vkEnumeratePhysicalDevices(inst.inst, &physical_count, &physical_device);
}