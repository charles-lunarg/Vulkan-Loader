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
#include "shims/shim.h"

namespace detail {
TestShim::TestShim() {
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
TestShim::~TestShim() {
    platform_shim->reset();
    platform_shim->clear_override();
}

MockDriverHandle::MockDriverHandle() {}
MockDriverHandle::MockDriverHandle(fs::path const& driver_name) {
    driver_library = LibraryWrapper(driver_name);

    auto sym = driver_library.get_symbol<GetMockDriverFunc>(NEW_MOCK_DRIVER_STR);

    proc_addr_mock_driver = sym;
}
MockDriver& MockDriverHandle::get_mock_driver() {
    assert(proc_addr_mock_driver != NULL && "symbol must be loaded before use");
    return proc_addr_mock_driver();
}
ManifestStores::ManifestStores(DebugMode debug_mode)
    : null(FRAMEWORK_BUILD_DIRECTORY, "null_dir",debug_mode),
      drivers(FRAMEWORK_BUILD_DIRECTORY, "driver_manifests", debug_mode),
      explicit_layers(FRAMEWORK_BUILD_DIRECTORY, "layer_manifests", debug_mode),
      implicit_layers(FRAMEWORK_BUILD_DIRECTORY, "layer_manifests", debug_mode) {}

}  // namespace detail
SingleDriverMockEnvironment::SingleDriverMockEnvironment(const char* driver_name_macro, DebugMode debug_mode )
    : test_shim(),
      vulkan_functions(),
      manifest_stores(debug_mode) {
    test_shim.platform_shim->redirect_all_paths(manifest_stores.null.location());

    test_shim.platform_shim->set_path(ManifestCategory::Driver, manifest_stores.drivers.location());
    test_shim.platform_shim->set_path(ManifestCategory::Explicit, manifest_stores.explicit_layers.location());
    test_shim.platform_shim->set_path(ManifestCategory::Implicit, manifest_stores.implicit_layers.location());

    driver_handle = detail::MockDriverHandle(driver_name_macro);

    ManifestICD icd_manifest;
    icd_manifest.lib_path = driver_name_macro;
    icd_manifest.api_version = VK_MAKE_VERSION(1, 0, 0);
    auto driver_loc = manifest_stores.drivers.write("mock_driver_icd.json", icd_manifest);
    test_shim.platform_shim->add_manifest(ManifestCategory::Driver, driver_loc);
}
MockDriver& SingleDriverMockEnvironment::get_mock_driver() { return driver_handle.get_mock_driver(); }
