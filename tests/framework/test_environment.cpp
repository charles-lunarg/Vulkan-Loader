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

#include "driver_defs.h"
#include "layer_defs.h"

#include "shim_config.h"
#include "shim/shim.h"

namespace detail {
PlatformShimWrapper::PlatformShimWrapper() {
#if defined(__linux__) || defined(__APPLE__)
    set_env_var("LD_PRELOAD", SHIM_LIBRARY_NAME);
#endif
    shim_library = LibraryWrapper(SHIM_LIBRARY_NAME);
    auto get_platform_shim_func = shim_library.get_symbol<PFN_get_platform_shim>(GET_PLATFORM_SHIM_STR);
    assert(get_platform_shim_func != NULL && "Must be able to get \"platform_shim\"");
    platform_shim = &get_platform_shim_func();
    platform_shim->setup_override();
    platform_shim->reset();
}
PlatformShimWrapper::~PlatformShimWrapper() {
    platform_shim->reset();
    platform_shim->clear_override();
}

TestICDHandle::TestICDHandle() {}
TestICDHandle::TestICDHandle(fs::path const& driver_name) {
    driver_library = LibraryWrapper(driver_name);
    proc_addr_get_test_icd = driver_library.get_symbol<GetNewTestICDFunc>(GET_TEST_ICD_FUNC_STR);
    proc_addr_get_new_test_icd = driver_library.get_symbol<GetNewTestICDFunc>(GET_NEW_TEST_ICD_FUNC_STR);
}
TestICD& TestICDHandle::get_test_icd() {
    assert(proc_addr_get_test_icd != NULL && "symbol must be loaded before use");
    return proc_addr_get_test_icd();
}
TestICD& TestICDHandle::get_new_test_icd() {
    assert(proc_addr_get_new_test_icd != NULL && "symbol must be loaded before use");
    return proc_addr_get_new_test_icd();
}
}  // namespace detail

FrameworkEnvironment::FrameworkEnvironment(DebugMode debug_mode)
    : platform_shim(),
      null_folder(FRAMEWORK_BUILD_DIRECTORY, "null_dir", debug_mode),
      drivers_folder(FRAMEWORK_BUILD_DIRECTORY, "driver_manifests", debug_mode),
      explicit_layers_folder(FRAMEWORK_BUILD_DIRECTORY, "layer_manifests", debug_mode),
      implicit_layers_folder(FRAMEWORK_BUILD_DIRECTORY, "layer_manifests", debug_mode),
      vulkan_functions() {
    platform_shim->redirect_all_paths(null_folder.location());

    platform_shim->set_path(ManifestCategory::Driver, drivers_folder.location());
    platform_shim->set_path(ManifestCategory::Explicit, explicit_layers_folder.location());
    platform_shim->set_path(ManifestCategory::Implicit, implicit_layers_folder.location());
}

SingleDriverShim::SingleDriverShim(DriverShimDetails driver_details, DebugMode debug_mode) : FrameworkEnvironment(debug_mode) {
    driver_handle = detail::TestICDHandle(driver_details.macro_name);

    ManifestICD icd_manifest;
    icd_manifest.lib_path = driver_details.macro_name;
    icd_manifest.api_version = driver_details.api_version;
    auto driver_loc = drivers_folder.write("test_icd_icd.json", icd_manifest);
    platform_shim->add_manifest(ManifestCategory::Driver, driver_loc);
}
TestICD& SingleDriverShim::get_test_icd() { return driver_handle.get_test_icd(); }

MultipleDriverShim::MultipleDriverShim(std::vector<DriverShimDetails> driver_name_macros, DebugMode debug_mode) : FrameworkEnvironment(debug_mode) {
    uint32_t i = 0;
    for (auto& driver_name : driver_name_macros) {
        drivers.push_back(detail::TestICDHandle(driver_name.macro_name));

        ManifestICD icd_manifest;
        icd_manifest.lib_path = driver_name.macro_name;
        icd_manifest.api_version = driver_name.api_version;
        auto driver_loc = drivers_folder.write(std::string("test_icd_") + std::to_string(i++) + ".json", icd_manifest);
        platform_shim->add_manifest(ManifestCategory::Driver, driver_loc);
    }
}
TestICD& MultipleDriverShim::get_test_icd(int i) { return drivers[i].get_test_icd(); }
TestICD& MultipleDriverShim::get_test_icd(std::string const& driver_name) {
    for (auto& driver : drivers) {
        if (driver.driver_name == driver_name) {
            return driver.get_test_icd();
        }
    }
    assert(false);
    return drivers[0].get_test_icd();
}
