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

#include "util/common.h"
#include "util/test_util.h"

#include "layer/layer_common.h"

#include "physical_device.h"


enum class CalledICDGIPA { not_called, vk_icd_gipa, vk_gipa };

enum class CalledNegotiateInterface { not_called, vk_icd_negotiate, vk_icd_gipa_first };

enum class InterfaceVersionCheck { not_called, loader_version_too_old, loader_version_too_new, icd_version_too_new, version_is_supported};

enum class CalledEnumerateAdapterPhysicalDevices { not_called, called, called_but_not_supported};

enum class UsingICDProvidedWSI { not_using, is_using};

struct TestICD {
    fs::path manifest_file_path;

    CalledICDGIPA called_vk_icd_gipa = CalledICDGIPA::not_called;
    CalledNegotiateInterface called_negotiate_interface = CalledNegotiateInterface::not_called;

    InterfaceVersionCheck interface_version_check = InterfaceVersionCheck::not_called;
    uint32_t min_icd_interface_version = 0; 
    uint32_t max_icd_interface_version = 6;
    uint32_t icd_interface_version_received = 0;

    CalledEnumerateAdapterPhysicalDevices called_enumerate_adapter_physical_devices = CalledEnumerateAdapterPhysicalDevices::not_called;

    bool enable_icd_wsi = false;
    UsingICDProvidedWSI is_using_icd_wsi = UsingICDProvidedWSI::not_using;

    uint32_t icd_api_version = VK_MAKE_VERSION(1, 0, 0);
    std::vector<Layer> instance_layers;
    std::vector<Extension> instance_extensions;
    std::vector<PhysicalDevice> physical_devices;

    DispatchableHandle<VkInstance> instance_handle;
    std::vector<DispatchableHandle<VkDevice>> device_handles;

    uint64_t created_surface_count = 0;
    uint64_t created_swapchain_count = 0;


    TestICD() {}
    ~TestICD() {}

    TestICD& SetMinICDInterfaceVersion(uint32_t icd_interface_version) {
        this->min_icd_interface_version = icd_interface_version;
        return *this;
    }
    TestICD& SetICDAPIVersion(uint32_t api_version) {
        this->icd_api_version = api_version;
        return *this;
    }
    TestICD& AddInstanceLayer(Layer layer) { return AddInstanceLayers({layer}); }

    TestICD& AddInstanceLayers(std::vector<Layer> layers) {
        this->instance_layers.reserve(layers.size() + this->instance_layers.size());
        for (auto& layer : layers) {
            this->instance_layers.push_back(layer);
        }
        return *this;
    }
    TestICD& AddInstanceExtension(Extension extension) { return AddInstanceExtensions({extension}); }

    TestICD& AddInstanceExtensions(std::vector<Extension> extensions) {
        this->instance_extensions.reserve(extensions.size() + this->instance_extensions.size());
        for (auto& extension : extensions) {
            this->instance_extensions.push_back(extension);
        }
        return *this;
    }

    PhysicalDevice& FindPhysDevice(std::string deviceName) {
        for (auto& phys_dev : physical_devices) {
            if (deviceName == phys_dev.properties.deviceName) {
                return phys_dev;
            }
        }
        assert(false && "Physical Device name not found!");
        return physical_devices[0];
    }
    PhysicalDevice& GetPhysDevice(VkPhysicalDevice physicalDevice) {
        for (auto& phys_dev : physical_devices) {
            if (phys_dev.vk_physical_device.handle == physicalDevice) return phys_dev;
        }
        assert(false && "vkPhysicalDevice not found!");
        return physical_devices[0];
    }

    InstanceCreateInfo GetVkInstanceCreateInfo() {
        InstanceCreateInfo info;
        for (auto& layer : instance_layers) info.enabled_layers.push_back(layer.layerName.data());
        for (auto& ext : instance_extensions) info.enabled_extensions.push_back(ext.extensionName.data());
        return info;
    }
    // DeviceCreateInfo GetDeviceCreateInfo(std::string deviceName) {
    //     DeviceCreateInfo info;
    //     auto phys_dev = FindPhysDevice(deviceName);
    //     for (auto& ext : phys_dev.extensions) info.enabled_extensions.push_back(ext.extensionName.data());
    //     for (auto& layer : phys_dev.layers) info.enabled_layers.push_back(layer.layerName.data());
    //     uint32_t i = 0;
    //     for (auto& queue_family : phys_dev.queue_family_properties) {
    //         DeviceQueueCreateInfo queue_detail{};
    //         queue_detail.queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    //         queue_detail.queue.queueFamilyIndex = i++;
    //         queue_detail.queue.flags = queue_family.queueFlags;
    //         queue_detail.queue.queueCount = queue_family.queueCount;
    //         queue_detail.priorities.resize(queue_family.queueCount, 0.5f);
    //         info.queue_info_details.push_back(queue_detail);
    //     }
    //     for (auto& queue_detail : info.queue_info_details) {
    //         queue_detail.queue.queueCount = queue_detail.priorities.size();
    //         queue_detail.queue.pQueuePriorities = queue_detail.priorities.data();
    //         info.queue_infos.push_back(queue_detail.queue);
    //     }
    //     return info;
    // }

};

using GetTestICDFunc = TestICD& (*)();
#define GET_TEST_ICD_FUNC_STR "get_test_icd_func"

using GetNewTestICDFunc = TestICD& (*)();
#define GET_NEW_TEST_ICD_FUNC_STR "get_new_test_icd_func"