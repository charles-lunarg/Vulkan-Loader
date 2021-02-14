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


#include "util/common.h"
#include "util/test_util.h"

#include "layer_util.h"

namespace test
{

struct layer_data {
    VkInstance instance;
    VkLayerInstanceDispatchTable *instance_dispatch_table;

    layer_data() : instance(VK_NULL_HANDLE), instance_dispatch_table(nullptr) {};
};

static uint32_t loader_layer_if_version = CURRENT_LOADER_LAYER_INTERFACE_VERSION;

static std::unordered_map<void *, layer_data *> layer_data_map;

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
		VkInstance* pInstance)
{
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    assert(chain_info != nullptr);

    assert(chain_info->u.pLayerInfo != nullptr);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    assert(fpGetInstanceProcAddr != nullptr);

    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance) fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == nullptr)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    VkLayerInstanceCreateInfo *create_dev_info = get_chain_info(pCreateInfo, VK_LOADER_LAYER_CREATE_DEVICE_CALLBACK);
    assert(create_dev_info != nullptr);
    auto layer_create_device = create_dev_info->u.layerDevice.pfnLayerCreateDevice;
    auto layer_destroy_device = create_dev_info->u.layerDevice.pfnLayerDestroyDevice;

    layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(*pInstance), layer_data_map);
    instance_data->instance = *pInstance;
    instance_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, instance_data->instance_dispatch_table, fpGetInstanceProcAddr);

    // uint32_t count = 0;
    // instance_data->instance_dispatch_table->EnumeratePhysicalDevices(*pInstance, &count, nullptr);
    // std::vector<VkPhysicalDevice> devices(count);
    // instance_data->instance_dispatch_table->EnumeratePhysicalDevices(*pInstance, &count, devices.data());
    // VkDevice device;
    // auto device_create_info = VkDeviceCreateInfo{
    //     VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  // sType
    //     nullptr,                               // pNext
    //     0,                                     // flags
    //     0,                                     // queueCreateInfoCount
    //     nullptr,                               // pQueueCreateInfos
    //     0,                                     // enabledLayerCount
    //     nullptr,                               // ppEnabledLayerNames
    //     0,                                     // enabledExtensionCount
    //     nullptr,                               // ppEnabledExtensionNames
    //     nullptr                                // pEnabledFeatures
    // };
    // auto deviceQueue = VkDeviceQueueCreateInfo{};
    // deviceQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // float priorities = 1;
    // deviceQueue.queueFamilyIndex = 0;
    // deviceQueue.queueCount = 1;
    // deviceQueue.pQueuePriorities = &priorities;
    // device_create_info.pQueueCreateInfos = &deviceQueue;
    // device_create_info.queueCreateInfoCount = 1;

    // PFN_vkGetDeviceProcAddr newGDPA = nullptr;
    // layer_create_device(*pInstance, devices[0], &device_create_info, nullptr, &device, vkGetInstanceProcAddr, &newGDPA);
    // assert(newGDPA != nullptr);
    // PFN_vkDestroyDevice destroy = (PFN_vkDestroyDevice)newGDPA(device, "vkDestroyDevice");
    // layer_destroy_device(device, nullptr, destroy);

    // std::cout << "VK_LAYER_LUNARG_test: device count " << count << '\n';

    // // Marker for testing.
    // std::cout << "VK_LAYER_LUNARG_test: CreateInstance" << '\n';

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    layer_data *instance_data = GetLayerDataPtr(key, layer_data_map);
    instance_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

    delete instance_data->instance_dispatch_table;
    layer_data_map.erase(key);

    // // Marker for testing.
    // std::cout << "VK_LAYER_LUNARG_test: DestroyInstance" << '\n';
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (strcmp("vkGetInstanceProcAddr", funcName)) return reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr);
    if (strcmp("vkCreateInstance", funcName)) return reinterpret_cast<PFN_vkVoidFunction>(CreateInstance);
    if (strcmp("vkDestroyInstance", funcName)) return reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance);

    // Only call down the chain for Vulkan commands that this layer does not intercept.
    layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = instance_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == nullptr)
    {
        return nullptr;
    }

    return pTable->GetInstanceProcAddr(instance, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
    assert(instance);

    layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pTable = instance_data->instance_dispatch_table;
    if (pTable->GetPhysicalDeviceProcAddr == nullptr)
    {
        return nullptr;
    }

    return pTable->GetPhysicalDeviceProcAddr(instance, funcName);
}

} //namespace test

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    return test::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
    return test::GetPhysicalDeviceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    assert(pVersionStruct != NULL);
    assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

    // Fill in the function pointers if our version is at least capable of having the structure contain them.
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = vk_layerGetPhysicalDeviceProcAddr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        test::loader_layer_if_version = pVersionStruct->loaderLayerInterfaceVersion;
    } else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}
