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

#include "layer_common.h"
#include "loader/generated/vk_dispatch_table_helper.h"

// For the given data key, look up the layer_data instance from given layer_data_map
template <typename DATA_T>
DATA_T *GetLayerDataPtr(void *data_key, std::unordered_map<void *, DATA_T *> &layer_data_map) {
    DATA_T *debug_data;
    typename std::unordered_map<void *, DATA_T *>::const_iterator got;

    /* TODO: We probably should lock here, or have caller lock */
    got = layer_data_map.find(data_key);

    if (got == layer_data_map.end()) {
        debug_data = new DATA_T;
        layer_data_map[(void *)data_key] = debug_data;
    } else {
        debug_data = got->second;
    }

    return debug_data;
}

template <typename DATA_T>
void FreeLayerDataPtr(void *data_key, std::unordered_map<void *, DATA_T *> &layer_data_map) {
    auto got = layer_data_map.find(data_key);
    assert(got != layer_data_map.end());

    delete got->second;
    layer_data_map.erase(got);
}

typedef std::unordered_map<void *, VkLayerDispatchTable *> device_table_map;
typedef std::unordered_map<void *, VkLayerInstanceDispatchTable *> instance_table_map;
VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa, device_table_map &map);
VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa);
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa, instance_table_map &map);
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa);

typedef void *dispatch_key;

static inline dispatch_key get_dispatch_key(const void *object) { return (dispatch_key) * (VkLayerDispatchTable **)object; }

VkLayerDispatchTable *device_dispatch_table(void *object);

VkLayerInstanceDispatchTable *instance_dispatch_table(void *object);

VkLayerDispatchTable *get_dispatch_table(device_table_map &map, void *object);

VkLayerInstanceDispatchTable *get_dispatch_table(instance_table_map &map, void *object);

VkLayerInstanceCreateInfo *get_chain_info(const VkInstanceCreateInfo *pCreateInfo, VkLayerFunction func);
VkLayerDeviceCreateInfo *get_chain_info(const VkDeviceCreateInfo *pCreateInfo, VkLayerFunction func);

void destroy_device_dispatch_table(dispatch_key key);
void destroy_instance_dispatch_table(dispatch_key key);
void destroy_dispatch_table(device_table_map &map, dispatch_key key);
void destroy_dispatch_table(instance_table_map &map, dispatch_key key);


static device_table_map tableMap;
static instance_table_map tableInstanceMap;

// Map lookup must be thread safe
VkLayerDispatchTable *device_dispatch_table(void *object) {
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = tableMap.find((void *)key);
    assert(it != tableMap.end() && "Not able to find device dispatch entry");
    return it->second;
}

VkLayerInstanceDispatchTable *instance_dispatch_table(void *object) {
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = tableInstanceMap.find((void *)key);
    assert(it != tableInstanceMap.end() && "Not able to find instance dispatch entry");
    return it->second;
}

void destroy_dispatch_table(device_table_map &map, dispatch_key key) {
    device_table_map::const_iterator it = map.find((void *)key);
    if (it != map.end()) {
        delete it->second;
        map.erase(it);
    }
}

void destroy_dispatch_table(instance_table_map &map, dispatch_key key) {
    instance_table_map::const_iterator it = map.find((void *)key);
    if (it != map.end()) {
        delete it->second;
        map.erase(it);
    }
}

void destroy_device_dispatch_table(dispatch_key key) { destroy_dispatch_table(tableMap, key); }

void destroy_instance_dispatch_table(dispatch_key key) { destroy_dispatch_table(tableInstanceMap, key); }

VkLayerDispatchTable *get_dispatch_table(device_table_map &map, void *object) {
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = map.find((void *)key);
    assert(it != map.end() && "Not able to find device dispatch entry");
    return it->second;
}

VkLayerInstanceDispatchTable *get_dispatch_table(instance_table_map &map, void *object) {
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = map.find((void *)key);
    assert(it != map.end() && "Not able to find instance dispatch entry");
    return it->second;
}

VkLayerInstanceCreateInfo *get_chain_info(const VkInstanceCreateInfo *pCreateInfo, VkLayerFunction func) {
    VkLayerInstanceCreateInfo *chain_info = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerInstanceCreateInfo *)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

VkLayerDeviceCreateInfo *get_chain_info(const VkDeviceCreateInfo *pCreateInfo, VkLayerFunction func) {
    VkLayerDeviceCreateInfo *chain_info = (VkLayerDeviceCreateInfo *)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerDeviceCreateInfo *)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CommandBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa, instance_table_map &map) {
    VkLayerInstanceDispatchTable *pTable;
    dispatch_key key = get_dispatch_key(instance);
    instance_table_map::const_iterator it = map.find((void *)key);

    if (it == map.end()) {
        pTable = new VkLayerInstanceDispatchTable;
        map[(void *)key] = pTable;
    } else {
        return it->second;
    }

    layer_init_instance_dispatch_table(instance, pTable, gpa);

    // Setup func pointers that are required but not externally exposed.  These won't be added to the instance dispatch table by
    // default.
    pTable->GetPhysicalDeviceProcAddr = (PFN_GetPhysicalDeviceProcAddr)gpa(instance, "vk_layerGetPhysicalDeviceProcAddr");

    return pTable;
}

VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa) {
    return initInstanceTable(instance, gpa, tableInstanceMap);
}

VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa, device_table_map &map) {
    VkLayerDispatchTable *pTable;
    dispatch_key key = get_dispatch_key(device);
    device_table_map::const_iterator it = map.find((void *)key);

    if (it == map.end()) {
        pTable = new VkLayerDispatchTable;
        map[(void *)key] = pTable;
    } else {
        return it->second;
    }

    layer_init_device_dispatch_table(device, pTable, gpa);

    return pTable;
}

VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa) {
    return initDeviceTable(device, gpa, tableMap);
}

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

FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    return test::GetInstanceProcAddr(instance, funcName);
}

FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}

FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties)
{
    return VK_ERROR_LAYER_NOT_PRESENT;
}

FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
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
