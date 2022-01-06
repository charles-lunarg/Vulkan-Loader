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

// Load the global function pointers with and without a NULL vkInstance handle.
// Call the function to make sure it is callable, don't care about what is returned.
TEST(GetProcAddr, GlobalFunctions) {
    FrameworkEnvironment env{};
    env.add_icd(TestICDDetails(TEST_ICD_PATH_VERSION_6));
    env.get_test_icd().physical_devices.emplace_back("physical_device_0");

    auto& gipa = env.vulkan_functions.vkGetInstanceProcAddr;
    // global entry points with NULL instance handle
    {
        auto EnumerateInstanceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(gipa(NULL, "vkEnumerateInstanceExtensionProperties"));
        handle_assert_has_value(EnumerateInstanceExtensionProperties);
        uint32_t ext_count = 0;
        ASSERT_EQ(VK_SUCCESS, EnumerateInstanceExtensionProperties("", &ext_count, nullptr));

        auto EnumerateInstanceLayerProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(gipa(NULL, "vkEnumerateInstanceLayerProperties"));
        handle_assert_has_value(EnumerateInstanceLayerProperties);
        uint32_t layer_count = 0;
        ASSERT_EQ(VK_SUCCESS, EnumerateInstanceLayerProperties(&layer_count, nullptr));

        auto EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(gipa(NULL, "vkEnumerateInstanceVersion"));
        handle_assert_has_value(EnumerateInstanceVersion);
        uint32_t api_version = 0;
        EnumerateInstanceVersion(&api_version);

        auto GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa(NULL, "vkGetInstanceProcAddr"));
        handle_assert_has_value(GetInstanceProcAddr);
        GetInstanceProcAddr(NULL, "vkGetInstanceProcAddr");

        auto CreateInstance = reinterpret_cast<PFN_vkCreateInstance>(gipa(NULL, "vkCreateInstance"));
        handle_assert_has_value(CreateInstance);
    }
    // Now create an instance and query the functions again - should work because the instance version is less than 1.2
    for (int i = 0; i <= 2; i++) {
        InstWrapper inst{env.vulkan_functions};
        inst.create_info.api_version = VK_MAKE_API_VERSION(0, 1, i, 0);
        inst.CheckCreate();

        auto EnumerateInstanceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(gipa(inst, "vkEnumerateInstanceExtensionProperties"));
        handle_assert_has_value(EnumerateInstanceExtensionProperties);
        uint32_t ext_count = 0;
        ASSERT_EQ(VK_SUCCESS, EnumerateInstanceExtensionProperties("", &ext_count, nullptr));

        auto EnumerateInstanceLayerProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(gipa(inst, "vkEnumerateInstanceLayerProperties"));
        handle_assert_has_value(EnumerateInstanceLayerProperties);
        uint32_t layer_count = 0;
        ASSERT_EQ(VK_SUCCESS, EnumerateInstanceLayerProperties(&layer_count, nullptr));

        auto EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(gipa(inst, "vkEnumerateInstanceVersion"));
        handle_assert_has_value(EnumerateInstanceVersion);
        uint32_t api_version = 0;
        EnumerateInstanceVersion(&api_version);

        auto GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa(inst, "vkGetInstanceProcAddr"));
        handle_assert_has_value(GetInstanceProcAddr);
        GetInstanceProcAddr(NULL, "vkGetInstanceProcAddr");

        auto CreateInstance = reinterpret_cast<PFN_vkCreateInstance>(gipa(inst, "vkCreateInstance"));
        handle_assert_has_value(CreateInstance);
    }
    {
        // Create a 1.3 instance - now everything should return NULL
        InstWrapper inst{env.vulkan_functions};
        inst.create_info.api_version = VK_MAKE_API_VERSION(0, 1, 3, 0);
        inst.CheckCreate();

        auto EnumerateInstanceExtensionProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(gipa(inst, "vkEnumerateInstanceExtensionProperties"));
        handle_assert_null(EnumerateInstanceExtensionProperties);

        auto EnumerateInstanceLayerProperties =
            reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(gipa(inst, "vkEnumerateInstanceLayerProperties"));
        handle_assert_null(EnumerateInstanceLayerProperties);

        auto EnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(gipa(inst, "vkEnumerateInstanceVersion"));
        handle_assert_null(EnumerateInstanceVersion);

        auto CreateInstance = reinterpret_cast<PFN_vkCreateInstance>(gipa(inst, "vkCreateInstance"));
        handle_assert_null(CreateInstance);

        auto GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa(inst, "vkGetInstanceProcAddr"));
        handle_assert_null(GetInstanceProcAddr);

        // get a non pre-instance function pointer
        auto EnumeratePhysicalDevices = reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa(inst, "vkEnumeratePhysicalDevices"));
        handle_assert_has_value(EnumeratePhysicalDevices);

        EnumeratePhysicalDevices = reinterpret_cast<PFN_vkGetInstanceProcAddr>(gipa(NULL, "vkEnumeratePhysicalDevices"));
        handle_assert_null(EnumeratePhysicalDevices);
    }
}

struct GetPhysicalDeviceProperties2ICDConfig {
    uint32_t api_version;
    bool supports_gpdp2KHR;
};

struct GetPhysicalDeviceProperties2Config {
    std::vector<GetPhysicalDeviceProperties2ICDConfig> icds;

    uint32_t inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    bool inst_supports_gpdp2KHR = false;

    VkResult inst_creation_return = VK_SUCCESS;

    bool has_func_props2KHR = false;
    bool has_func_props2 = false;
};

void CheckGetPhysicalDeviceProperties2Config(GetPhysicalDeviceProperties2Config const& config) {
    FrameworkEnvironment env{};
    int icd_index = 0;
    for (auto& icd : config.icds) {
        env.add_icd(TestICDDetails(TEST_ICD_PATH_VERSION_6));
        env.get_test_icd().physical_devices.emplace_back(std::string("physical_device_") + std::to_string(icd_index++));
        env.get_test_icd().set_icd_api_version(icd.api_version);
        if (icd.supports_gpdp2KHR) {
            env.get_test_icd().add_instance_extension(Extension{VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME});
        }
    }
    InstWrapper inst{env.vulkan_functions};
    inst.create_info.api_version = config.inst_version;
    if (config.inst_supports_gpdp2KHR) {
        inst.create_info.add_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
    ASSERT_NO_FATAL_FAILURE(inst.CheckCreate(config.inst_creation_return));
    if (config.inst_creation_return != VK_SUCCESS) {
        return;
    }
    auto phys_devs = inst.GetPhysDevs(config.icds.size());
    for (auto const& phys_dev : phys_devs) {
        VkPhysicalDeviceProperties2KHR props2KHR_s{};
        auto props2KHR = (PFN_vkGetPhysicalDeviceProperties2)env.vulkan_functions.vkGetInstanceProcAddr(
            inst, "vkGetPhysicalDeviceProperties2KHR");
        if (config.has_func_props2KHR) {
            ASSERT_NO_FATAL_FAILURE(handle_assert_has_value(props2KHR));
            props2KHR(phys_dev, &props2KHR_s);
        } else {
            ASSERT_NO_FATAL_FAILURE(handle_assert_null(props2KHR));
        }

        VkPhysicalDeviceProperties2 props2_s{};
        auto props2 =
            (PFN_vkGetPhysicalDeviceProperties2)env.vulkan_functions.vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties2");
        if (config.has_func_props2) {
            ASSERT_NO_FATAL_FAILURE(handle_assert_has_value(props2));
            props2(phys_dev, &props2_s);
        } else {
            ASSERT_NO_FATAL_FAILURE(handle_assert_null(props2));
        }
    }
}

TEST(GetPhysicalDeviceProperties2, SupportMatrix) {
    GetPhysicalDeviceProperties2Config a{};
    // icd doesn't support ext
    a.icds = {{VK_MAKE_API_VERSION(0, 1, 0, 0), false}};

    // Create a 1.0 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;                                             // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));  // CRASH on props2

    // Create a 1.0 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_ERROR_EXTENSION_NOT_PRESENT;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;
    // ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));  // CRASH on props2

    // Create a 1.1 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_ERROR_EXTENSION_NOT_PRESENT;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // icd now supports ext
    a.icds = {{VK_MAKE_API_VERSION(0, 1, 0, 0), true}};

    // Create a 1.0 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;  // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.0 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;  // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // now there are two icds, one that is 1.1 and one that is 1.0 without ext support
    a.icds = {{VK_MAKE_API_VERSION(0, 1, 1, 0), true}, {VK_MAKE_API_VERSION(0, 1, 0, 0), false}};

    // Create a 1.0 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;  // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.0 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;  // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // now there are two icds, one that is 1.0 with ext support and one that is 1.1 with ext support
    a.icds = {{VK_MAKE_API_VERSION(0, 1, 0, 0), true}, {VK_MAKE_API_VERSION(0, 1, 1, 0), true}};

    // Create a 1.0 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.0 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;  // not good...
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = false;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = false;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));

    // Create a 1.1 instance with ext support
    a.inst_version = VK_MAKE_API_VERSION(0, 1, 1, 0);
    a.inst_supports_gpdp2KHR = true;
    a.inst_creation_return = VK_SUCCESS;
    a.has_func_props2KHR = true;
    a.has_func_props2 = true;
    ASSERT_NO_FATAL_FAILURE(CheckGetPhysicalDeviceProperties2Config(a));
}
