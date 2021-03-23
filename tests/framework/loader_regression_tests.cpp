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

class RegressionTests : public ::testing::Test {
   protected:
    virtual void SetUp() {
        env = std::unique_ptr<SingleDriverShim>(
            new SingleDriverShim(TestICDDetails(TEST_ICD_PATH_VERSION_2, VK_MAKE_VERSION(1, 0, 0))));
    }

    virtual void TearDown() { env.reset(); }
    std::unique_ptr<SingleDriverShim> env;
};

TEST_F(RegressionTests, CreateInstance_BasicRun) {
    auto& driver = env->get_test_icd();
    driver.SetDriverAPIVersion(VK_MAKE_VERSION(1, 0, 0));
    driver.SetMinICDInterfaceVersion(5);

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);
}

TEST_F(RegressionTests, CreateInstance_DestroyInstanceNullHandle) {
    env->vulkan_functions.fp_vkDestroyInstance(VK_NULL_HANDLE, nullptr);
}

TEST_F(RegressionTests, CreateInstance_DestroyDeviceNullHandle) {
    env->vulkan_functions.fp_vkDestroyDevice(VK_NULL_HANDLE, nullptr);
}

TEST_F(RegressionTests, CreateInstance_ExtensionNotPresent) {
    auto& driver = env->get_test_icd();
    {
        VkInstance inst = VK_NULL_HANDLE;
        auto inst_info = driver.GetVkInstanceCreateInfo();
        inst_info.add_extension("VK_EXT_validation_features");  // test icd won't report this as supported
        ASSERT_EQ(VK_ERROR_EXTENSION_NOT_PRESENT,
                  env->vulkan_functions.fp_vkCreateInstance(inst_info.get(), VK_NULL_HANDLE, &inst));
    }
    {
        VkInstance inst = VK_NULL_HANDLE;
        auto inst_info = driver.GetVkInstanceCreateInfo();
        inst_info.add_extension("Non_existant_extension");  // unknown instance extension
        ASSERT_EQ(VK_ERROR_EXTENSION_NOT_PRESENT,
                  env->vulkan_functions.fp_vkCreateInstance(inst_info.get(), VK_NULL_HANDLE, &inst));
    }
}

TEST_F(RegressionTests, CreateInstance_LayerNotPresent) {
    auto& driver = env->get_test_icd();

    VkInstance inst = VK_NULL_HANDLE;
    auto inst_info = driver.GetVkInstanceCreateInfo();
    inst_info.add_layer("non_existant_layer");

    ASSERT_EQ(VK_ERROR_LAYER_NOT_PRESENT, env->vulkan_functions.fp_vkCreateInstance(inst_info.get(), VK_NULL_HANDLE, &inst));
}
/*
TEST_F(RegressionTests, CreateInstance_LayerPresent) {
    std::string layer_name = "ExistenceLayer";
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = layer_name;
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer_manifest;
    layer_manifest.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer_manifest, "ExistenceLayer.json");

    auto& driver = env->get_test_icd();
    Layer layer{layer_name, VK_MAKE_VERSION(1, 0, 0), VK_MAKE_VERSION(1, 0, 0), layer_name, {}};
    driver.AddInstanceLayer(layer);

    InstWrapper inst{env->vulkan_functions};
    auto inst_info = driver.GetVkInstanceCreateInfo();
    inst_info.add_layer(layer_name.c_str());
    ASSERT_EQ(CreateInst(inst, inst_info), VK_SUCCESS);
}
*/
TEST_F(RegressionTests, EnumeratePhysicalDevices_OneCall) {
    auto& driver = env->get_test_icd();
    driver.SetMinICDInterfaceVersion(5);

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.emplace_back("physical_device_1");
    driver.physical_devices.emplace_back("physical_device_2");
    driver.physical_devices.emplace_back("physical_device_3");

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    uint32_t physical_count = driver.physical_devices.size();
    uint32_t returned_physical_count = driver.physical_devices.size();
    std::vector<VkPhysicalDevice> physical_device_handles = std::vector<VkPhysicalDevice>(physical_count);
    ASSERT_EQ(VK_SUCCESS, inst->fp_vkEnumeratePhysicalDevices(inst, &returned_physical_count, physical_device_handles.data()));
    ASSERT_EQ(physical_count, returned_physical_count);
}

TEST_F(RegressionTests, EnumeratePhysicalDevices_TwoCall) {
    auto& driver = env->get_test_icd();
    driver.SetMinICDInterfaceVersion(5);

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.emplace_back("physical_device_1");

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    uint32_t physical_count = driver.physical_devices.size();
    uint32_t returned_physical_count = 0;
    ASSERT_EQ(VK_SUCCESS, inst->fp_vkEnumeratePhysicalDevices(inst.inst, &returned_physical_count, nullptr));
    ASSERT_EQ(physical_count, returned_physical_count);

    std::unique_ptr<VkPhysicalDevice[]> physical_device_handles(new VkPhysicalDevice[physical_count]);
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumeratePhysicalDevices(inst.inst, &returned_physical_count,
                                                                              physical_device_handles.get()));
    ASSERT_EQ(physical_count, returned_physical_count);
}

TEST_F(RegressionTests, EnumeratePhysicalDevices_MatchOneAndTwoCallNumbers) {
    auto& driver = env->get_test_icd();
    driver.SetMinICDInterfaceVersion(5);

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.emplace_back("physical_device_1");
    driver.physical_devices.emplace_back("physical_device_2");

    InstWrapper inst1{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst1, inst_create_info), VK_SUCCESS);

    uint32_t physical_count_one_call = driver.physical_devices.size();
    uint32_t returned_physical_count_one_call = driver.physical_devices.size();
    std::unique_ptr<VkPhysicalDevice[]> physical_device_handles_one_call(new VkPhysicalDevice[physical_count_one_call]);
    ASSERT_EQ(VK_SUCCESS, inst1->fp_vkEnumeratePhysicalDevices(inst1, &returned_physical_count_one_call,
                                                               physical_device_handles_one_call.get()));
    ASSERT_EQ(physical_count_one_call, returned_physical_count_one_call);

    InstWrapper inst2{env->vulkan_functions};
    ASSERT_EQ(CreateInst(inst2, inst_create_info), VK_SUCCESS);

    uint32_t physical_count = driver.physical_devices.size();
    uint32_t returned_physical_count = 0;
    ASSERT_EQ(VK_SUCCESS, inst2->fp_vkEnumeratePhysicalDevices(inst2, &returned_physical_count, nullptr));
    ASSERT_EQ(physical_count, returned_physical_count);

    std::unique_ptr<VkPhysicalDevice[]> physical_device_handles(new VkPhysicalDevice[physical_count]);
    ASSERT_EQ(VK_SUCCESS, inst2->fp_vkEnumeratePhysicalDevices(inst2, &returned_physical_count, physical_device_handles.get()));
    ASSERT_EQ(physical_count, returned_physical_count);

    ASSERT_EQ(returned_physical_count, returned_physical_count);
}

TEST_F(RegressionTests, EnumeratePhysicalDevices_TwoCallIncomplete) {
    auto& driver = env->get_test_icd();
    driver.SetMinICDInterfaceVersion(5);

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.emplace_back("physical_device_1");

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    uint32_t physical_count = 0;
    ASSERT_EQ(VK_SUCCESS, inst->fp_vkEnumeratePhysicalDevices(inst, &physical_count, nullptr));
    ASSERT_EQ(physical_count, driver.physical_devices.size());

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physical_count]);

    // Remove one from the physical device count so we can get the VK_INCOMPLETE message
    physical_count = 1;

    ASSERT_EQ(VK_INCOMPLETE, inst->fp_vkEnumeratePhysicalDevices(inst, &physical_count, physical.get()));
    ASSERT_EQ(physical_count, 1);
}

// Test to make sure that layers enabled in the instance show up in the list of device layers.
// TODO: make layers actually work
/*
TEST_F(RegressionTests, EnumerateDeviceLayers_LayersMatch) {
    std::string layer_name = "ExistenceLayer";
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = layer_name;
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer_manifest;
    layer_manifest.layers.push_back(layer_desc);
    env->AddExplicitLayer(layer_manifest, "ExistenceLayer.json");

    auto& driver = env->get_test_icd();
    driver.physical_devices.emplace_back("physical_device_0");
    driver.AddInstanceLayer(Layer(layer_name));

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    inst_create_info.add_layer(layer_name.c_str());
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    VkPhysicalDevice physical_device;
    CreatePhysDev(inst, physical_device);

    uint32_t layer_count = 0;
    VkLayerProperties layer_props;
    inst->fp_vkEnumerateDeviceLayerProperties(physical_device, &layer_count, &layer_props);
    ASSERT_EQ(layer_count, 1);
    ASSERT_EQ(strcmp(layer_props.layerName, layer_name.c_str()), 0);
}
*/

TEST_F(RegressionTests, CreateDevice_ExtensionNotPresent) {
    auto& driver = env->get_test_icd();

    MockQueueFamilyProperties family_props{{VK_QUEUE_GRAPHICS_BIT, 1, 0, {1, 1, 1}}, true};

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.back().queue_family_properties.push_back(family_props);

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    VkPhysicalDevice phys_dev;
    ASSERT_EQ(CreatePhysDev(inst, phys_dev), VK_SUCCESS);

    uint32_t familyCount = 0;
    inst->fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &familyCount, nullptr);
    ASSERT_EQ(familyCount, 1);

    VkQueueFamilyProperties families;
    inst->fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &familyCount, &families);
    ASSERT_EQ(familyCount, 1);
    ASSERT_EQ(families, family_props.properties);

    DeviceCreateInfo dev_create_info;
    dev_create_info.add_extension("NotPresent");
    DeviceQueueCreateInfo queue_info;
    queue_info.add_priority(0.0f);
    dev_create_info.add_device_queue(queue_info);

    VkDevice device;
    ASSERT_EQ(VK_ERROR_EXTENSION_NOT_PRESENT, inst->fp_vkCreateDevice(phys_dev, dev_create_info.get(), nullptr, &device));
}

TEST_F(RegressionTests, CreateDevice_LayersNotPresent) {
    auto& driver = env->get_test_icd();

    MockQueueFamilyProperties family_props{{VK_QUEUE_GRAPHICS_BIT, 1, 0, {1, 1, 1}}, true};

    driver.physical_devices.emplace_back("physical_device_0");
    driver.physical_devices.back().queue_family_properties.push_back(family_props);

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    VkPhysicalDevice phys_dev;
    ASSERT_EQ(CreatePhysDev(inst, phys_dev), VK_SUCCESS);

    uint32_t familyCount = 0;
    inst->fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &familyCount, nullptr);
    ASSERT_EQ(familyCount, 1);

    VkQueueFamilyProperties families;
    inst->fp_vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &familyCount, &families);
    ASSERT_EQ(familyCount, 1);
    ASSERT_EQ(families, family_props.properties);

    DeviceCreateInfo dev_create_info;
    dev_create_info.add_layer("NotPresent");
    DeviceQueueCreateInfo queue_info;
    queue_info.add_priority(0.0f);
    dev_create_info.add_device_queue(queue_info);

    VkDevice device;
    ASSERT_EQ(VK_SUCCESS, inst->fp_vkCreateDevice(phys_dev, dev_create_info.get(), nullptr, &device));
}

// TODO: Layer support
TEST_F(RegressionTests, EnumerateInstanceLayerProperties_PropertyCountLessThanAvailable) {
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = "ExistenceLayer";
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer;
    layer.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer, "test_layer_1.json");
    env->AddExplicitLayer(layer, "test_layer_2.json");

    uint32_t layer_count = 0;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    ASSERT_EQ(layer_count, 2);

    std::array<VkLayerProperties, 2> layers;
    layer_count = 1;  // artificially remove one layer
    ASSERT_EQ(VK_INCOMPLETE, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));
    ASSERT_EQ(layer_count, 1);
    ASSERT_EQ(layers[0], layer_desc.get_layer_properties());
    ASSERT_NE(layers[1], layer_desc.get_layer_properties());
}

/*
TEST_F(RegressionTests, EnumerateDeviceLayerProperties_PropertyCountLessThanAvailable){
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = "ExistenceLayer";
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer;
    layer.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer, "test_layer_1.json");
    env->AddExplicitLayer(layer, "test_layer_2.json");

    MockQueueFamilyProperties family_props{{VK_QUEUE_GRAPHICS_BIT, 1, 0, {1, 1, 1}}, true};

    auto& driver = env->get_test_icd();
    driver.physical_devices.emplace_back("physical_device_0");

    InstWrapper inst{env->vulkan_functions};
    InstanceCreateInfo inst_create_info;
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    VkPhysicalDevice phys_dev;
    ASSERT_EQ(CreatePhysDev(inst, phys_dev), VK_SUCCESS);

    uint32_t layer_count = 0;
    ASSERT_EQ(VK_SUCCESS, inst->fp_vkEnumerateDeviceLayerProperties(phys_dev, &layer_count, nullptr));
    ASSERT_EQ(layer_count, 2);

    std::array<VkLayerProperties, 2> layers;
    layer_count = 1; //artificially remove one layer

    ASSERT_EQ(VK_INCOMPLETE, inst->fp_vkEnumerateDeviceLayerProperties(phys_dev, &layer_count, layers.data()));
    ASSERT_EQ(layer_count, 1);
}
*/

TEST_F(RegressionTests, EnumerateInstanceLayerProperties_Count) {
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = "ExistenceLayer";
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer;
    layer.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer, "test_layer_1.json");
    env->AddExplicitLayer(layer, "test_layer_2.json");

    uint32_t layer_count = 0;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    ASSERT_EQ(layer_count, 2);
}

TEST_F(RegressionTests, EnumerateInstanceLayerProperties_OnePass) {
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = "ExistenceLayer";
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer;
    layer.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer, "test_layer_1.json");
    env->AddExplicitLayer(layer, "test_layer_2.json");

    uint32_t layer_count = 2;
    std::array<VkLayerProperties, 2> layers;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));
    ASSERT_EQ(layer_count, 2);
    ASSERT_EQ(layers[0], layer_desc.get_layer_properties());
    ASSERT_EQ(layers[1], layer_desc.get_layer_properties());
}

TEST_F(RegressionTests, EnumerateInstanceLayerProperties_TwoPass) {
    ManifestLayer::LayerDescription layer_desc;
    layer_desc.name = "ExistenceLayer";
    layer_desc.description = "Test layer";
    layer_desc.lib_path = TEST_LAYER_PATH_EXISTENCE;

    ManifestLayer layer;
    layer.layers.push_back(layer_desc);

    env->AddExplicitLayer(layer, "test_layer_1.json");
    env->AddExplicitLayer(layer, "test_layer_2.json");

    uint32_t layer_count = 0;
    std::array<VkLayerProperties, 2> layers;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    ASSERT_EQ(layer_count, 2);

    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));
    ASSERT_EQ(layer_count, 2);
    ASSERT_EQ(layers[0], layer_desc.get_layer_properties());
    ASSERT_EQ(layers[1], layer_desc.get_layer_properties());
}

TEST_F(RegressionTests, EnumerateInstanceExtensionProperties_PropertyCountLessThanAvailable) {
    uint32_t extension_count = 0;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, nullptr));
    ASSERT_EQ(extension_count, 2); //return debug report & debug utils
    extension_count = 1;  // artificially remove one extension

    std::array<VkExtensionProperties, 2> extensions;
    ASSERT_EQ(VK_INCOMPLETE,
              env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, extensions.data()));
    ASSERT_EQ(extension_count, 1);
    // loader always adds the debug report & debug utils extensions
    ASSERT_EQ(strcmp(extensions[0].extensionName, "VK_EXT_debug_report"), 0);
}


TEST_F(RegressionTests, EnumerateInstanceExtensionProperties_FilterUnkownInstanceExtensions) {
    Extension first_ext{"FirstTestExtension"};
    Extension second_ext{"SecondTestExtension"};
    env->get_new_test_icd().AddInstanceExtensions({first_ext, second_ext});

    uint32_t extension_count = 0;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, nullptr));
    ASSERT_EQ(extension_count, 2); //return debug report & debug utils
    
    std::array<VkExtensionProperties, 2> extensions;
    ASSERT_EQ(VK_SUCCESS,
              env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, extensions.data()));
    ASSERT_EQ(extension_count, 2);
    // loader always adds the debug report & debug utils extensions
    ASSERT_EQ(strcmp(extensions[0].extensionName, "VK_EXT_debug_report"), 0);
    ASSERT_EQ(strcmp(extensions[1].extensionName, "VK_EXT_debug_utils"), 0);
}

TEST_F(RegressionTests, EnumerateInstanceExtensionProperties_DisableUnknownInstanceExtensionFiltering) {
    Extension first_ext{"FirstTestExtension"};
    Extension second_ext{"SecondTestExtension"};
    env->get_new_test_icd().AddInstanceExtensions({first_ext, second_ext});

    set_env_var("VK_LOADER_DISABLE_INST_EXT_FILTER","1");

    uint32_t extension_count = 0;
    ASSERT_EQ(VK_SUCCESS, env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, nullptr));
    ASSERT_EQ(extension_count, 4);
    
    std::array<VkExtensionProperties, 4> extensions;
    ASSERT_EQ(VK_SUCCESS,
              env->vulkan_functions.fp_vkEnumerateInstanceExtensionProperties("", &extension_count, extensions.data()));
    ASSERT_EQ(extension_count, 4);

    ASSERT_EQ(extensions[0], first_ext.get());
    ASSERT_EQ(extensions[1], second_ext.get());
    //Loader always adds these two extensions
    ASSERT_EQ(strcmp(extensions[2].extensionName, "VK_EXT_debug_report"), 0);
    ASSERT_EQ(strcmp(extensions[3].extensionName, "VK_EXT_debug_utils"), 0);
}