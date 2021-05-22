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

#include "test_environment.h"

TEST(PortabilityICDConfiguration, PortabilityICDOnly) {
    SingleICDShim env(TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), true), DebugMode::none);

    TestICD& icd0 = env.get_test_icd();
    icd0.physical_devices.emplace_back("physical_device_0");
    icd0.max_icd_interface_version = 1;
    {  // enable portability extension and flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        VkPhysicalDevice phys_dev = nullptr;
        uint32_t phys_dev_count = 1;
        ASSERT_EQ(env.vulkan_functions.fp_vkEnumeratePhysicalDevices(inst, &phys_dev_count, &phys_dev), VK_SUCCESS);
        ASSERT_EQ(phys_dev_count, 1);
        ASSERT_NE(phys_dev, nullptr);
    }
    {  // enable portability flag but not extension
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_ERROR_INCOMPATIBLE_DRIVER);
    }
    {  // enable portability extension but not flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_ERROR_INCOMPATIBLE_DRIVER);
    }
    {  // enable neither the portability extension or the flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.inst_info.flags = 0;  // make sure its 0 - no portability
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_ERROR_INCOMPATIBLE_DRIVER);
    }
}

void CheckEnumeratePhysicalDevice(MultipleICDShim& env, InstWrapper& inst, uint32_t physical_count) {
    uint32_t returned_physical_count = 0;
    ASSERT_EQ(VK_SUCCESS, env.vulkan_functions.fp_vkEnumeratePhysicalDevices(inst.inst, &returned_physical_count, nullptr));
    ASSERT_EQ(returned_physical_count, physical_count);

    std::vector<VkPhysicalDevice> physical_device_handles(physical_count);
    ASSERT_EQ(VK_SUCCESS, env.vulkan_functions.fp_vkEnumeratePhysicalDevices(inst.inst, &returned_physical_count,
                                                                             physical_device_handles.data()));
    for (uint32_t i = 0; i < physical_count; i++) {
        ASSERT_NE(physical_device_handles[i], nullptr);
    }
}

TEST(PortabilityICDConfiguration, PortabilityAndRegularICD) {
    MultipleICDShim env({TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), false),
                         TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), true)},
                        DebugMode::none);

    TestICD& icd0 = env.get_test_icd(0);
    TestICD& icd1 = env.get_test_icd(1);

    icd0.physical_devices.emplace_back("physical_device_0");
    icd0.max_icd_interface_version = 1;

    icd1.physical_devices.emplace_back("portability_physical_device_1");
    icd1.max_icd_interface_version = 1;
    {  // enable portability extension and flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 2);
    }
    {  // enable portability extension but not flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 1);
    }
    {  // enable portability flag but not extension
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 1);
    }
    {  // do not enable portability extension or flag
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 1);
    }
}

#if defined(WIN32)
// Version 6 provides a mechanism to allow the loader to sort physical devices.
// The loader will only attempt to sort physical devices on an ICD if version 6 of the interface is supported.
// This version provides the vk_icdEnumerateAdapterPhysicalDevices function.
TEST(PortabilityICDConfiguration, no_device_sorting) {
    MultipleICDShim env({TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), false),
                         TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), true)},
                        DebugMode::none);

    auto& regular_icd = env.get_test_icd(0);
    auto& portable_icd = env.get_test_icd(1);

    regular_icd.physical_devices.emplace_back("physical_device_0");
    portable_icd.physical_devices.emplace_back("physical_device_1");
    regular_icd.min_icd_interface_version = 5;
    portable_icd.min_icd_interface_version = 5;

    InstWrapper inst{env.vulkan_functions};
    InstanceCreateInfo inst_create_info;
    inst_create_info.add_extension("VK_KHR_portability_enumeration");
    inst_create_info.inst_info.flags = 1;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    CheckEnumeratePhysicalDevice(env, inst, 2);
    ASSERT_EQ(regular_icd.called_enumerate_adapter_physical_devices, CalledEnumerateAdapterPhysicalDevices::not_called);
    ASSERT_EQ(portable_icd.called_enumerate_adapter_physical_devices, CalledEnumerateAdapterPhysicalDevices::not_called);
}

TEST(PortabilityICDConfiguration, dxgi_portability_drivers) {
    MultipleICDShim env(
        {TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES, VK_MAKE_VERSION(1, 0, 0), false),
         TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES, VK_MAKE_VERSION(1, 0, 0), true)},
        DebugMode::none);

    auto& regular_icd = env.get_test_icd(0);
    auto& portable_icd = env.get_test_icd(1);

    regular_icd.physical_devices.emplace_back("physical_device_0");
    portable_icd.physical_devices.emplace_back("physical_device_1");
    regular_icd.min_icd_interface_version = 6;
    portable_icd.min_icd_interface_version = 6;

    uint32_t regular_driver_index = 2;  // which drive this test pretends to be
    auto& regular_known_driver = known_driver_list.at(regular_driver_index);
    DXGI_ADAPTER_DESC1 regular_desc1{};
    wcsncpy(&regular_desc1.Description[0], L"TestDriver1", 128);
    regular_desc1.VendorId = regular_known_driver.vendor_id;
    regular_desc1.AdapterLuid;
    regular_desc1.Flags = DXGI_ADAPTER_FLAG_NONE;
    env.platform_shim->add_dxgi_adapter(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES, GpuType::discrete,
                                        regular_driver_index, regular_desc1);

    uint32_t portable_driver_index = 3;  // which drive this test pretends to be

    // auto& portable_known_driver = known_driver_list.at(portable_driver_index);
    // DXGI_ADAPTER_DESC1 portable_desc1{};
    // wcsncpy(&portable_desc1.Description[0], L"TestDriver2", 128);
    // portable_desc1.VendorId = portable_known_driver.vendor_id;
    // portable_desc1.AdapterLuid;
    // portable_desc1.Flags = DXGI_ADAPTER_FLAG_NONE;
    // env.platform_shim->add_dxgi_adapter(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES, GpuType::discrete,
    //                                     portable_driver_index, portable_desc1);
    {
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 2);
        ASSERT_EQ(regular_icd.called_enumerate_adapter_physical_devices, CalledEnumerateAdapterPhysicalDevices::called);
        ASSERT_EQ(portable_icd.called_enumerate_adapter_physical_devices, CalledEnumerateAdapterPhysicalDevices::called);
    }
    env.ResetCalledState();
    {
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 1);
        ASSERT_EQ(regular_icd.called_enumerate_adapter_physical_devices, CalledEnumerateAdapterPhysicalDevices::called);
    }
}
TEST(PortabilityICDConfiguration, EnumAdapters2) {
    MultipleICDShim env({TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), false, false),
                         TestICDDetails(TEST_ICD_PATH_VERSION_2_EXPORT_ICD_GPDPA, VK_MAKE_VERSION(1, 0, 0), true, false)},
                        DebugMode::none);

    auto& regular_icd = env.get_test_icd(0);
    auto& portable_icd = env.get_test_icd(1);

    regular_icd.physical_devices.emplace_back("physical_device_0");
    portable_icd.physical_devices.emplace_back("physical_device_1");
    regular_icd.min_icd_interface_version = 5;
    portable_icd.min_icd_interface_version = 5;

    SHIM_D3DKMT_ADAPTERINFO regular_d3dkmt_adapter_info{};
    regular_d3dkmt_adapter_info.hAdapter = 0;  //
    regular_d3dkmt_adapter_info.AdapterLuid = _LUID{10, 1000};
    regular_d3dkmt_adapter_info.NumOfSources = 1;
    regular_d3dkmt_adapter_info.bPresentMoveRegionsPreferred = true;

    D3DKMT_Adapter adapter;
    adapter.info = regular_d3dkmt_adapter_info;
    adapter.icds = {env.get_test_icd_manifest_path(0)};
    env.platform_shim->add_d3dkmt_adapter(adapter);

    SHIM_D3DKMT_ADAPTERINFO portability_d3dkmt_adapter_info{};
    portability_d3dkmt_adapter_info.hAdapter = 1;  //
    portability_d3dkmt_adapter_info.AdapterLuid = _LUID{11, 1001};
    portability_d3dkmt_adapter_info.NumOfSources = 1;
    portability_d3dkmt_adapter_info.bPresentMoveRegionsPreferred = true;

    D3DKMT_Adapter portability_adapter;
    portability_adapter.info = portability_d3dkmt_adapter_info;
    portability_adapter.portability_icds = {env.get_test_icd_manifest_path(1)};
    env.platform_shim->add_d3dkmt_adapter(portability_adapter);

    {
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        inst_create_info.add_extension("VK_KHR_portability_enumeration");
        inst_create_info.inst_info.flags = 1;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 2);
    }
    env.ResetCalledState();
    {
        InstWrapper inst{env.vulkan_functions};
        InstanceCreateInfo inst_create_info;
        ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

        CheckEnumeratePhysicalDevice(env, inst, 1);
    }
}
#endif  // defined(WIN32)
