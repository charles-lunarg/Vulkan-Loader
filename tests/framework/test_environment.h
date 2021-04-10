#pragma once

// Must be first to guard against GTest and Xlib colliding due to redefinitions of "None" and "Bool"
#include "util/include_gtest.h"
#include "util/common.h"
#include "util/test_util.h"

#include "shim/shim.h"

#include "icd/physical_device.h"
#include "icd/test_icd.h"

#include "framework_config.h"
#include "icd_defs.h"
#include "layer_defs.h"

namespace detail {
struct PlatformShimWrapper {
    PlatformShimWrapper(DebugMode debug_mode = DebugMode::none);
    ~PlatformShimWrapper();
    PlatformShimWrapper(PlatformShimWrapper const&) = delete;
    PlatformShimWrapper& operator=(PlatformShimWrapper const&) = delete;

    // Convenience
    PlatformShim* operator->() { return platform_shim; }

    LibraryWrapper shim_library;
    PlatformShim* platform_shim;
    DebugMode debug_mode = DebugMode::none;
};

struct TestICDHandle {
    TestICDHandle();
    TestICDHandle(fs::path const& icd_path);
    TestICD& get_new_test_icd();
    TestICD& get_test_icd();
    fs::path get_icd_full_path();

    // Must use statically
    LibraryWrapper icd_library;
    GetTestICDFunc proc_addr_get_test_icd;
    GetNewTestICDFunc proc_addr_get_new_test_icd;
};
}  // namespace detail

struct TestICDDetails {
    TestICDDetails(const char* icd_path, uint32_t api_version = VK_MAKE_VERSION(1, 0, 0))
        : icd_path(icd_path), api_version(api_version) {}
    const char* icd_path = nullptr;
    uint32_t api_version = VK_MAKE_VERSION(1, 0, 0);
};
struct FrameworkEnvironment {
    FrameworkEnvironment(DebugMode debug_mode = DebugMode::none);

    void AddICD(TestICDDetails icd_details, const std::string& json_name);
    void AddImplicitLayer(ManifestLayer layer_manifest, const std::string& json_name);
    void AddExplicitLayer(ManifestLayer layer_manifest, const std::string& json_name);

    detail::PlatformShimWrapper platform_shim;
    fs::FolderManager null_folder;
    fs::FolderManager icd_folder;
    fs::FolderManager explicit_layer_folder;
    fs::FolderManager implicit_layer_folder;
    VulkanFunctions vulkan_functions;
};

struct SingleICDShim : FrameworkEnvironment {
    SingleICDShim(TestICDDetails icd_details, DebugMode debug_mode = DebugMode::none);

    TestICD& get_test_icd();
    TestICD& get_new_test_icd();

    fs::path get_test_icd_path();

    detail::TestICDHandle icd_handle;
};

struct MultipleICDShim : FrameworkEnvironment {
    MultipleICDShim(std::vector<TestICDDetails> icd_details_vector, DebugMode debug_mode = DebugMode::none);

    TestICD& get_test_icd(int index);
    TestICD& get_new_test_icd(int index);
    fs::path get_test_icd_path(int index);

    std::vector<detail::TestICDHandle> icds;
};