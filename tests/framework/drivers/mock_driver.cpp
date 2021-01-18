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

#include "mock_driver.h"

/*
Cmake driven macro defines

conditionally export vk_icdGetInstanceProcAddr
MOCK_DRIVER_EXPORT_ICD_GIPA

conditionally export vk_icdNegotiateLoaderICDInterfaceVersion
MOCK_DRIVER_EXPORT_NEGOTIATE_INTERFACE_VERSION

conditionally export vk_icdGetPhysicalDeviceProcAddr
MOCK_DRIVER_EXPORT_ICD_GPDPA

conditionally export vk_icdEnumerateAdapterPhysicalDevices
MOCK_DRIVER_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES
*/

MockDriver driver;
extern "C" {
FRAMEWORK_EXPORT MockDriver& new_mock_driver() {
    driver.~MockDriver();
    return *(new (&driver) MockDriver());
}
}

namespace detail {
Layer& FindLayer(std::vector<Layer>& layers, std::string layerName) {
    for (auto& layer : layers) {
        if (layer.layerName == layerName) return layer;
    }
    assert(false && "Layer name not found!");
    return layers[0];
}
bool CheckLayer(std::vector<Layer>& layers, std::string layerName) {
    for (auto& layer : layers) {
        if (layer.layerName == layerName) return true;
    }
    return false;
}

// typename T must have '.get()' function that returns a type U
template <typename T, typename U>
VkResult FillCountPtr(std::vector<T> const& data_vec, uint32_t* pCount, U* pData) {
    if (pCount == nullptr) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    if (pData == nullptr) {
        *pCount = data_vec.size();
        return VK_SUCCESS;
    }
    uint32_t amount_written = 0;
    uint32_t amount_to_write = data_vec.size();
    if (*pCount < data_vec.size()) {
        amount_to_write = *pCount;
    }
    for (size_t i = 0; i < amount_to_write; i++) {
        pData[i] = data_vec[i].get();
        amount_written++;
    }
    if (*pCount < data_vec.size()) {
        *pCount = amount_written;
        return VK_INCOMPLETE;
    }
    *pCount = amount_written;
    return VK_SUCCESS;
}

template <typename T>
VkResult FillCountPtr(std::vector<T> const& data_vec, uint32_t* pCount, T* pData) {
    if (pCount == nullptr) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    if (pData == nullptr) {
        *pCount = data_vec.size();
        return VK_SUCCESS;
    }
    uint32_t amount_written = 0;
    uint32_t amount_to_write = data_vec.size();
    if (*pCount < data_vec.size()) {
        amount_to_write = *pCount;
    }
    for (size_t i = 0; i < amount_to_write; i++) {
        pData[i] = data_vec[i];
        amount_written++;
    }
    if (*pCount < data_vec.size()) {
        *pCount = amount_written;
        return VK_INCOMPLETE;
    }
    *pCount = amount_written;
    return VK_SUCCESS;
}
}  // namespace detail

//// Instance Functions ////

// VK_SUCCESS,VK_INCOMPLETE
VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount,
                                                                           VkExtensionProperties* pProperties) {
    if (pLayerName != nullptr) {
        auto& layer = detail::FindLayer(driver.instance_layers, std::string(pLayerName));
        return detail::FillCountPtr(layer.extensions, pPropertyCount, pProperties);
    } else {  // instance extensions
        detail::FillCountPtr(driver.instance_extensions, pPropertyCount, pProperties);
    }

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
    return detail::FillCountPtr(driver.instance_layers, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
    if (pApiVersion != nullptr) {
        *pApiVersion = VK_MAKE_VERSION(1, 0, 0);
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                     const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    if (pCreateInfo == nullptr || pCreateInfo->pApplicationInfo == nullptr) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    if (driver.icd_api_version < VK_MAKE_VERSION(1, 1, 0)) {
        if (pCreateInfo->pApplicationInfo->apiVersion > VK_MAKE_VERSION(1, 0, 0)) {
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }
    }
    // VK_SUCCESS
    *pInstance = driver.instance_handle.handle;

    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL mock_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {}

// VK_SUCCESS,VK_INCOMPLETE
VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount,
                                                               VkPhysicalDevice* pPhysicalDevices) {
    if (pPhysicalDevices == nullptr) {
        *pPhysicalDeviceCount = driver.physical_devices.size();
    } else {
        uint32_t handles_written = 0;
        for (size_t i = 0; i < driver.physical_devices.size(); i++) {
            if (i < *pPhysicalDeviceCount) {
                handles_written++;
                pPhysicalDevices[i] = driver.physical_devices[i].vk_physical_device.handle;
            } else {
                *pPhysicalDeviceCount = handles_written;
                return VK_INCOMPLETE;
            }
        }
        *pPhysicalDeviceCount = handles_written;
    }
    return VK_SUCCESS;
}

//// Physical Device functions ////

// VK_SUCCESS,VK_INCOMPLETE
VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,
                                                                     VkLayerProperties* pProperties) {
    assert(false && "Drivers don't contain layers???");
    // auto phys_dev = driver.GetPhysDevice(physicalDevice);
    // if (pProperties == nullptr) {
    //     *pPropertyCount = phys_dev.layers.size();
    // } else {
    //     return detail::FillCountPtr(phys_dev.layers, pPropertyCount, pProperties);
    // }
    return VK_SUCCESS;
}

// VK_SUCCESS, VK_INCOMPLETE, VK_ERROR_LAYER_NOT_PRESENT
VKAPI_ATTR VkResult VKAPI_CALL mock_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName,
                                                                         uint32_t* pPropertyCount,
                                                                         VkExtensionProperties* pProperties) {
    auto& phys_dev = driver.GetPhysDevice(physicalDevice);
    if (pLayerName != nullptr) {
        assert(false && "Drivers don't contain layers???");
        // bool is_present = detail::CheckLayer(phys_dev.layers, pLayerName);
        // if (!is_present) return VK_ERROR_LAYER_NOT_PRESENT;
        // auto& layer = detail::FindLayer(phys_dev.layers, pLayerName);
        // return detail::FillCountPtr(layer.extensions, pPropertyCount, pProperties);
        return VK_SUCCESS;
    } else {  // instance extensions
        return detail::FillCountPtr(phys_dev.extensions, pPropertyCount, pProperties);
    }
}

VKAPI_ATTR void VKAPI_CALL mock_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                         uint32_t* pQueueFamilyPropertyCount,
                                                                         VkQueueFamilyProperties* pQueueFamilyProperties) {
    auto& phys_dev = driver.GetPhysDevice(physicalDevice);
    detail::FillCountPtr(phys_dev.queue_family_properties, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
                                                   const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    // VK_SUCCESS
    auto device_handle = DispatchableHandle<VkDevice>();
    *pDevice = device_handle.handle;
    driver.device_handles.emplace_back(std::move(device_handle));
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL mock_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    auto found = std::find(driver.device_handles.begin(), driver.device_handles.end(), device);
    if (found != driver.device_handles.end()) driver.device_handles.erase(found);
}

//// WSI
#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    return VK_SUCCESS;
}
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateMetalSurfaceEXT(VkInstance instance, const VkMetalSurfaceCreateInfoEXT* pCreateInfo,
                                                            const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    return VK_SUCCESS;
}
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    return VK_SUCCESS;
}
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
                                                          const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    return VK_SUCCESS;
}
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                                                           const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    return VK_SUCCESS;
}
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                                            const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) {
    std::cout << "Hit create win32 surf\n";
    if (nullptr != pSurface) {
        *pSurface = reinterpret_cast<VkSurfaceKHR>(++driver.created_surface_count);
    }
    return VK_SUCCESS;
}
#endif

VKAPI_ATTR void VKAPI_CALL mock_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
                                                    const VkAllocationCallbacks* pAllocator) {
    // Nothing to do
}
VKAPI_ATTR VkResult VKAPI_CALL mock_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo,
                                                         const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    if (nullptr != pSwapchain) {
        *pSwapchain = reinterpret_cast<VkSwapchainKHR>(++driver.created_swapchain_count);
    }
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL mock_vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                                         VkSurfaceKHR surface, VkBool32* pSupported) {
    if (nullptr != pSupported) {
        *pSupported = driver.GetPhysDevice(physicalDevice).queue_family_properties.at(queueFamilyIndex).support_present;
    }
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL mock_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                              VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) {
    if (nullptr != pSurfaceCapabilities) {
        *pSurfaceCapabilities = driver.GetPhysDevice(physicalDevice).surface_capabilities;
    }
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL mock_vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                         uint32_t* pSurfaceFormatCount,
                                                                         VkSurfaceFormatKHR* pSurfaceFormats) {
    detail::FillCountPtr(driver.GetPhysDevice(physicalDevice).surface_formats, pSurfaceFormatCount, pSurfaceFormats);
    return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL mock_vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                              uint32_t* pPresentModeCount,
                                                                              VkPresentModeKHR* pPresentModes) {
    detail::FillCountPtr(driver.GetPhysDevice(physicalDevice).surface_present_modes, pPresentModeCount, pPresentModes);
    return VK_SUCCESS;
}

//// stubs

VKAPI_ATTR VkResult VKAPI_CALL mock_stub_func_with_return() { return VK_SUCCESS; }
VKAPI_ATTR void VKAPI_CALL mock_stub_func_no_return() {}

//// trampolines

#define TO_VOID_PFN(func) reinterpret_cast<PFN_vkVoidFunction>(func)

PFN_vkVoidFunction get_instance_func_ver_1_1(VkInstance instance, const char* pName) {
    if (driver.icd_api_version < VK_MAKE_VERSION(1, 1, 0)) {
        if (strcmp(pName, "mock_vkEnumerateInstanceVersion") == 0) {
            return TO_VOID_PFN(mock_vkEnumerateInstanceVersion);
        }
    }
    return nullptr;
}
PFN_vkVoidFunction get_instance_func_ver_1_2(VkInstance instance, const char* pName) {
    if (driver.icd_api_version < VK_MAKE_VERSION(1, 2, 0)) {
        return nullptr;
    }
    return nullptr;
}

PFN_vkVoidFunction get_instance_func_wsi(VkInstance instance, const char* pName) {
    if (driver.min_icd_interface_version >= 3 && driver.enable_icd_wsi  == true) {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pName, "vkCreateAndroidSurfaceKHR") == 0) {
            driver.is_using_icd_wsi = UsingICDProvidedWSI::is_using;
            return TO_VOID_PFN(mock_vkCreateAndroidSurfaceKHR);
        }
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        if (strcmp(pName, "vkCreateMetalSurfaceEXT") == 0) {
            return TO_VOID_PFN(mock_vkCreateMetalSurfaceEXT);
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pName, "vkCreateWaylandSurfaceKHR") == 0) {
            return TO_VOID_PFN(mock_vkCreateWaylandSurfaceKHR);
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pName, "vkCreateXcbSurfaceKHR") == 0) {
            return TO_VOID_PFN(mock_vkCreateXcbSurfaceKHR);
        }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pName, "vkCreateXlibSurfaceKHR") == 0) {
            return TO_VOID_PFN(mock_vkCreateXlibSurfaceKHR);
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pName, "vkCreateWin32SurfaceKHR") == 0) {
            return TO_VOID_PFN(mock_vkCreateWin32SurfaceKHR);
        }
#endif
        if (strcmp(pName, "vkDestroySurfaceKHR") == 0) {
            driver.is_using_icd_wsi = UsingICDProvidedWSI::is_using;
            return TO_VOID_PFN(mock_vkDestroySurfaceKHR);
        }
    }

    if (strcmp(pName, "vkGetPhysicalDeviceSurfaceSupportKHR") == 0) return TO_VOID_PFN(mock_vkGetPhysicalDeviceSurfaceSupportKHR);
    if (strcmp(pName, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") == 0)
        return TO_VOID_PFN(mock_vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    if (strcmp(pName, "vkGetPhysicalDeviceSurfaceFormatsKHR") == 0) return TO_VOID_PFN(mock_vkGetPhysicalDeviceSurfaceFormatsKHR);
    if (strcmp(pName, "vkGetPhysicalDeviceSurfacePresentModesKHR") == 0)
        return TO_VOID_PFN(mock_vkGetPhysicalDeviceSurfacePresentModesKHR);

    return nullptr;
}

PFN_vkVoidFunction get_instance_func(VkInstance instance, const char* pName) {
    if (strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0)
        return TO_VOID_PFN(mock_vkEnumerateInstanceExtensionProperties);
    if (strcmp(pName, "vkEnumerateInstanceLayerProperties") == 0) return TO_VOID_PFN(mock_vkEnumerateInstanceLayerProperties);
    if (strcmp(pName, "vkCreateInstance") == 0) return TO_VOID_PFN(mock_vkCreateInstance);
    if (strcmp(pName, "vkDestroyInstance") == 0) return TO_VOID_PFN(mock_vkDestroyInstance);
    if (strcmp(pName, "vkEnumeratePhysicalDevices") == 0) return TO_VOID_PFN(mock_vkEnumeratePhysicalDevices);
    // if (strcmp(pName, "vk_icdEnumerateAdapterPhysicalDevices") == 0) return
    // TO_VOID_PFN(mock_vk_icdEnumerateAdapterPhysicalDevices);
    if (strcmp(pName, "vkEnumerateDeviceLayerProperties") == 0) return TO_VOID_PFN(mock_vkEnumerateDeviceLayerProperties);
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) return TO_VOID_PFN(mock_vkEnumerateDeviceExtensionProperties);
    if (strcmp(pName, "vkGetPhysicalDeviceQueueFamilyProperties") == 0)
        return TO_VOID_PFN(mock_vkGetPhysicalDeviceQueueFamilyProperties);
    if (strcmp(pName, "vkCreateDevice") == 0) return TO_VOID_PFN(mock_vkCreateDevice);

    if (strcmp(pName, "vkGetPhysicalDeviceFeatures") == 0 || strcmp(pName, "vkGetPhysicalDeviceProperties") == 0 ||
        strcmp(pName, "vkGetPhysicalDeviceMemoryProperties") == 0 ||
        strcmp(pName, "vkGetPhysicalDeviceSparseImageFormatProperties") == 0 ||
        strcmp(pName, "vkGetPhysicalDeviceFormatProperties") == 0)
        return TO_VOID_PFN(mock_stub_func_no_return);
    if (strcmp(pName, "vkGetPhysicalDeviceImageFormatProperties") == 0) return TO_VOID_PFN(mock_stub_func_with_return);

    PFN_vkVoidFunction ret_1_1 = get_instance_func_ver_1_1(instance, pName);
    if (ret_1_1 != nullptr) return ret_1_1;

    PFN_vkVoidFunction ret_1_2 = get_instance_func_ver_1_2(instance, pName);
    if (ret_1_2 != nullptr) return ret_1_2;

    PFN_vkVoidFunction ret_wsi = get_instance_func_wsi(instance, pName);
    if (ret_wsi != nullptr) return ret_wsi;

    return nullptr;
}

PFN_vkVoidFunction get_device_func(VkDevice device, const char* pName) {
    if (strcmp(pName, "vkDestroyDevice") == 0) return TO_VOID_PFN(mock_vkDestroyDevice);
    if (strcmp(pName, "vkCreateSwapchainKHR") == 0) return TO_VOID_PFN(mock_vkCreateSwapchainKHR);

    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL mock_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return get_instance_func(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL mock_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return get_device_func(device, pName);
}

PFN_vkVoidFunction base_get_instance_proc_addr(VkInstance instance, const char* pName) {
    if (pName == nullptr) return nullptr;
    if (instance == NULL) {
        if (strcmp(pName, "vkGetInstanceProcAddr") == 0) return TO_VOID_PFN(mock_vkGetInstanceProcAddr);
        if (strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0)
            return TO_VOID_PFN(mock_vkEnumerateInstanceExtensionProperties);
        if (strcmp(pName, "vkEnumerateInstanceLayerProperties") == 0) return TO_VOID_PFN(mock_vkEnumerateInstanceLayerProperties);
        if (strcmp(pName, "vkEnumerateInstanceVersion") == 0) return TO_VOID_PFN(mock_vkEnumerateInstanceVersion);
    }
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) return TO_VOID_PFN(mock_vkGetDeviceProcAddr);

    auto instance_func_return = get_instance_func(instance, pName);
    if (instance_func_return != nullptr) return instance_func_return;
    return nullptr;
}

// Exported functions
extern "C" {
#if defined(MOCK_DRIVER_EXPORT_NEGOTIATE_INTERFACE_VERSION)
extern FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion) {
    if (driver.called_vk_icd_gipa == CalledICDGIPA::not_called &&
        driver.called_negotiate_interface == CalledNegotiateInterface::not_called)
        driver.called_negotiate_interface = CalledNegotiateInterface::vk_icd_negotiate;
    else if (driver.called_vk_icd_gipa != CalledICDGIPA::not_called)
        driver.called_negotiate_interface = CalledNegotiateInterface::vk_icd_gipa_first;

    // loader puts the minimum it supports in pSupportedVersion, if that is lower than our minimum
    // If the ICD doesn't supports the interface version provided by the loader, report VK_ERROR_INCOMPATIBLE_DRIVER
    if (driver.min_icd_interface_version > *pSupportedVersion) {
        driver.interface_version_check = InterfaceVersionCheck::loader_version_too_old;
        *pSupportedVersion = driver.min_icd_interface_version;
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    // the loader-provided interface version is newer than that supported by the ICD
    if (driver.max_icd_interface_version < *pSupportedVersion) {
        driver.interface_version_check = InterfaceVersionCheck::loader_version_too_new;
        *pSupportedVersion = driver.max_icd_interface_version;
    }
    // ICD interface version is greater than the loader's,  return the loader's version
    else if (driver.max_icd_interface_version > *pSupportedVersion) {
        driver.interface_version_check = InterfaceVersionCheck::icd_version_too_new;
        // don't change *pSupportedVersion
    } else {
        driver.interface_version_check = InterfaceVersionCheck::version_is_supported;
        *pSupportedVersion = driver.max_icd_interface_version;
    }

    return VK_SUCCESS;
}
#endif
#if defined(MOCK_DRIVER_EXPORT_ICD_GPDPA)
FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(VkInstance instance, const char* pName) {
    // std::cout << "icdGetPhysicalDeviceProcAddr: " << pName << "\n";

    return nullptr;
}
#endif
#if defined(MOCK_DRIVER_EXPORT_ICD_GIPA)
FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    // std::cout << "icdGetInstanceProcAddr: " << pName << "\n";

    if (driver.called_vk_icd_gipa == CalledICDGIPA::not_called) driver.called_vk_icd_gipa = CalledICDGIPA::vk_icd_gipa;

    return base_get_instance_proc_addr(instance, pName);
    return nullptr;
}
#else
FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    // std::cout << "icdGetInstanceProcAddr: " << pName << "\n";

    if (driver.called_vk_icd_gipa == CalledICDGIPA::not_called) driver.called_vk_icd_gipa = CalledICDGIPA::vk_gipa;
    return base_get_instance_proc_addr(instance, pName);
}
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                                 const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    return mock_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                                                                       uint32_t* pPropertyCount,
                                                                                       VkExtensionProperties* pProperties) {
    return mock_vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}
#endif  // MOCK_DRIVER_EXPORT_ICD_GIPA

#if defined(MOCK_DRIVER_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES)
#if defined(WIN32)
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vk_icdEnumerateAdapterPhysicalDevices(VkInstance instance, LUID adapterLUID,
                                                                                      uint32_t* pPhysicalDeviceCount,
                                                                                      VkPhysicalDevice* pPhysicalDevices) {
    driver.called_enumerate_adapter_physical_devices = CalledEnumerateAdapterPhysicalDevices::called;
    return mock_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);

    return VK_SUCCESS;
}
#endif  // defined(WIN32)
#endif  // MOCK_DRIVER_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES

}  // extern "C"