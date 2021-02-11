#pragma once

#include "util/common.h"
#include "util/test_util.h"

#include "util/include_gtest.h"

#include "shim/shim.h"

#include "icd/physical_device.h"
#include "icd/test_icd.h"

#include "framework_config.h"

namespace detail {
struct PlatformShimWrapper {
    PlatformShimWrapper();
    ~PlatformShimWrapper();
    PlatformShimWrapper(PlatformShimWrapper const&) = delete;
    PlatformShimWrapper& operator=(PlatformShimWrapper const&) = delete;

    // Convenience
    PlatformShim* operator->() { return platform_shim; }

    LibraryWrapper shim_library;
    PlatformShim* platform_shim;
};

struct TestICDHandle {
    TestICDHandle();
    TestICDHandle(fs::path const& driver_name);
    TestICD& get_new_test_icd();
    TestICD& get_test_icd();

    // Must use statically
    std::string driver_name;
    LibraryWrapper driver_library;
    GetTestICDFunc proc_addr_get_test_icd;
    GetNewTestICDFunc proc_addr_get_new_test_icd;

};
}  // namespace detail
struct FrameworkEnvironment {
    FrameworkEnvironment(DebugMode debug_mode = DebugMode::none);

    detail::PlatformShimWrapper platform_shim;
    fs::FolderManager null_folder;
    fs::FolderManager drivers_folder;
    fs::FolderManager explicit_layers_folder;
    fs::FolderManager implicit_layers_folder;
    VulkanFunctions vulkan_functions;
};
struct DriverShimDetails {
    DriverShimDetails(const char* macro_name, uint32_t api_version = VK_MAKE_VERSION(1, 0, 0))
        : macro_name(macro_name), api_version(api_version) {}
    const char* macro_name = nullptr;
    uint32_t api_version = VK_MAKE_VERSION(1, 0, 0);
};

struct SingleDriverShim : FrameworkEnvironment {
    SingleDriverShim(DriverShimDetails driver_details, DebugMode debug_mode = DebugMode::none);

    TestICD& get_test_icd();

    // FrameworkEnvironment env;

    detail::TestICDHandle driver_handle;
};

struct MultipleDriverShim : FrameworkEnvironment {
    MultipleDriverShim(std::vector<DriverShimDetails> driver_macro_names, DebugMode debug_mode = DebugMode::none);

    TestICD& get_test_icd(int driver);
    TestICD& get_test_icd(std::string const& driver_name);

    std::vector<detail::TestICDHandle> drivers;
};