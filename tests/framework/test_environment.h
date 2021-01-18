#pragma once

#include "util/common.h"
#include "util/test_util.h"

#include "util/include_gtest.h"

#include "shims/shim.h"

#include "drivers/physical_device.h"
#include "drivers/mock_driver.h"

#include "framework_config.h"

namespace detail {
struct PlatformShimWrapper {
    PlatformShimWrapper();
    ~PlatformShimWrapper();
    PlatformShimWrapper(PlatformShimWrapper const&) = delete;
    PlatformShimWrapper& operator=(PlatformShimWrapper const&) = delete;

    //Convenience
    PlatformShim* operator->() { return platform_shim; }

    LibraryWrapper shim_library;
    PlatformShim* platform_shim;
};

struct MockDriverHandle {
    MockDriverHandle();
    MockDriverHandle(fs::path const& driver_name);
    MockDriver& get_mock_driver();

    LibraryWrapper driver_library;
    GetMockDriverFunc proc_addr_mock_driver;
};

struct ManifestStores {
    ManifestStores(DebugMode debug_mode);
    fs::FolderManager null;
    fs::FolderManager drivers;
    fs::FolderManager explicit_layers;
    fs::FolderManager implicit_layers;
};
}
struct SingleDriverMockEnvironment{
    SingleDriverMockEnvironment(const char* driver_name_macro, DebugMode debug_mode = DebugMode::none);

    MockDriver& get_mock_driver();

    detail::PlatformShimWrapper platform_shim;
    VulkanFunctions vulkan_functions;
    detail::MockDriverHandle driver_handle;
    detail::ManifestStores manifest_stores;
};