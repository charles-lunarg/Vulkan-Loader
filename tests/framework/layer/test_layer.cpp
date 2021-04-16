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

#include "test_layer.h"

TestLayer layer;
extern "C" {
FRAMEWORK_EXPORT TestLayer& get_test_layer_func() { return layer; }
FRAMEWORK_EXPORT TestLayer& get_new_test_layer_func() {
    layer.~TestLayer();
    return *(new (&layer) TestLayer());
}
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL base_get_instance_proc_addr(VkInstance instance, const char* pName) {
    if (pName == nullptr) return nullptr;
    if (instance == NULL) {
        if (strcmp(pName, "vkGetInstanceProcAddr") == 0) return TO_VOID_PFN(test_vkGetInstanceProcAddr);
        if (strcmp(pName, "vkEnumerateInstanceExtensionProperties") == 0)
            return TO_VOID_PFN(test_vkEnumerateInstanceExtensionProperties);
        if (strcmp(pName, "vkEnumerateInstanceLayerProperties") == 0) return TO_VOID_PFN(test_vkEnumerateInstanceLayerProperties);
        if (strcmp(pName, "vkEnumerateInstanceVersion") == 0) return TO_VOID_PFN(test_vkEnumerateInstanceVersion);
    }
    if (strcmp(pName, "vkGetDeviceProcAddr") == 0) return TO_VOID_PFN(test_vkGetDeviceProcAddr);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL get_device_func(VkDevice device, const char* pName) {
    if (strcmp(pName, "vkDestroyDevice") == 0) return TO_VOID_PFN(test_vkDestroyDevice);
    if (strcmp(pName, "vkCreateSwapchainKHR") == 0) return TO_VOID_PFN(test_vkCreateSwapchainKHR);

    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance, const char* pName) {
    // std::cout << "icdGetPhysicalDeviceProcAddr: " << pName << "\n";

    return nullptr;
}

extern "C" {

FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount,
                                                                                   VkLayerProperties* pProperties) {}
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* pLayerName,
                                                                                       uint32_t* pPropertyCount,
                                                                                       VkExtensionProperties* pProperties) {}

FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                                                 uint32_t* pPropertyCount,
                                                                                 VkLayerProperties* pProperties) {}
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice                            physicalDevice,
    const char*                                 pLayerName,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties{
        
    }

#if defined(TEST_LAYER_EXPORT_LAYER_NAMED_GIPA) || 1
#endif
#if defined(TEST_LAYER_EXPORT_NO_NAME_GIPA) || 1
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL test_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return get_instance_func(instance, pName);
}

#endif
#if defined(TEST_LAYER_EXPORT_LAYER_NAMED_GDPA) || 1
#endif
#if defined(TEST_LAYER_EXPORT_NO_NAME_GDPA) || 1
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL test_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return get_device_func(device, pName);
}
#endif

#if defined(LAYER_EXPORT_NEGOTIATE_LOADER_LAYER_INTERFACE_VERSION) || 1
FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(
                   VkNegotiateLayerInterface *pVersionStruct){
    if (pVersionStruct) {
        *pVersionStruct.loaderLayerInterfaceVersion = layer.implementation_version;
        *pVersionStruct.pfnGetInstanceProcAddr = &base_get_instance_proc_addr;
        *pVersionStruct.pfnGetDeviceProcAddr = &get_device_func;
        *pVersionStruct.pfnGetPhysicalDeviceProcAddr = &vk_layerGetPhysicalDeviceProcAddr;
        return VK_SUCCESS;
    }
    return VK_ERROR_INITIALIZATION_FAILED;
}
#endif

}  // extern "C"