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

TEST(PortabilityICDConfiguration, DifferentICDInterfaceVersions) {
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

        VkPhysicalDevice phys_dev = VK_NULL_HANDLE;
        uint32_t phys_dev_count = 1;
        ASSERT_EQ(env.vulkan_functions.fp_vkEnumeratePhysicalDevices(inst, &phys_dev_count, &phys_dev), VK_SUCCESS);
        ASSERT_EQ(phys_dev_count, 1);
        ASSERT_NE(phys_dev, VK_NULL_HANDLE);
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
