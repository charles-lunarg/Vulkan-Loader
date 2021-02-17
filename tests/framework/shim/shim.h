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

#if defined(WIN32)
#include <strsafe.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devpkey.h>
#include <winternl.h>

#define CINTERFACE
#include <dxgi1_6.h>
#include <adapters.h>
#endif

enum class ManifestCategory { Implicit, Explicit, Driver };
enum class GPUPreference { unspecified, minimum_power, high_performance };

#if defined(WIN32)
struct DXGIDriver {
    DXGIDriver(fs::path const& manifest_path, GPUPreference gpu_preference)
        : manifest_path(manifest_path.str()), gpu_preference(gpu_preference) {}
    std::string manifest_path;
    GPUPreference gpu_preference = GPUPreference::unspecified;
};

struct SHIM_D3DKMT_ADAPTERINFO {
    UINT hAdapter;
    LUID AdapterLuid;
    ULONG NumOfSources;
    BOOL bPresentMoveRegionsPreferred;
};
struct D3DKMT_Adapter {
    std::vector<SHIM_D3DKMT_ADAPTERINFO> adapters;
    std::vector<LoaderQueryRegistryInfo> registry_info;
};

#endif
// Necessary to have inline definitions as shim is a dll and thus functions defined in the .cpp
// wont be found by the rest of the application
struct PlatformShim {
    // Test Framework interface
    void setup_override(DebugMode debug_mode = DebugMode::none);
    void clear_override(DebugMode debug_mode = DebugMode::none);

    void reset(DebugMode debug_mode = DebugMode::none);

    void redirect_all_paths(fs::path const& path);

    void set_path(ManifestCategory category, fs::path const& path);

    void add_manifest(ManifestCategory category, fs::path const& path);

// platform specific shim interface
#if defined(WIN32)
    void add_dxgi_manifest(DXGIDriver const& driver);
    void add_dxgi_adapter_desc1(DXGI_ADAPTER_DESC1 desc1);
    void add_D3DKMT_ADAPTERINFO(D3DKMT_Adapter adapter);

    std::vector<std::string> drivers;
    std::vector<DXGIDriver> dxgi_drivers;
    std::vector<DXGI_ADAPTER_DESC1> dxgi_adapter_desc1s;
    std::vector<D3DKMT_Adapter> d3dkmt_adapters;

    uint32_t random_base_path = 0;

#elif defined(__linux__) || defined(__APPLE__)
    bool is_fake_path(fs::path const& path);
    fs::path const& get_fake_path(fs::path const& path);

    std::unordered_map<std::string, fs::path> redirection_map;
#endif
   private:
    void redirect_category(fs::path const& new_path, ManifestCategory category);
#if defined(WIN32)
#elif defined(__linux__) || defined(__APPLE__)
    void add(fs::path const& path, fs::path const& new_path);
    void remove(fs::path const& path);
#endif
};

std::vector<std::string> parse_env_var_list(std::string const& var);

using PFN_get_platform_shim = PlatformShim& (*)();
#define GET_PLATFORM_SHIM_STR "get_platform_shim"

// Functions which are called by the Test Framework need a definition, but since the shim is a DLL, this
// would require loading the functions before using. To get around this, most PlatformShim functions are member
// functions, so only the `get_platform_shim()` is needed to be loaded. As a consequence, all the member functions
// need to be defined inline, so that both framework code and shim dlls can use them.

inline void PlatformShim::redirect_all_paths(fs::path const& path) {
    redirect_category(path, ManifestCategory::Implicit);
    redirect_category(path, ManifestCategory::Explicit);
    redirect_category(path, ManifestCategory::Driver);
}

inline std::vector<std::string> parse_env_var_list(std::string const& var) {
    std::vector<std::string> items;
    int start = 0;
    int len = 0;
    for (size_t i = 0; i < var.size(); i++) {
#if defined(WIN32)
        if (var[i] == ';') {
#elif defined(__linux__) || defined(__APPLE__)
        if (var[i] == ':') {
#endif
            items.push_back(var.substr(start, len));
            start = i + 1;
            len = 0;
        } else {
            len++;
        }
    }
    items.push_back(var.substr(start, len));

    return items;
}

#if defined(WIN32)

inline std::string to_str(ManifestCategory category) {
    if (category == ManifestCategory::Implicit) return "ImplicitLayers";
    if (category == ManifestCategory::Explicit)
        return "ExplicitLayers";
    else
        return "Drivers";
}

inline const char* win_api_error_str(LSTATUS status) {
    if (status == ERROR_INVALID_FUNCTION) return "ERROR_INVALID_FUNCTION";
    if (status == ERROR_FILE_NOT_FOUND) return "ERROR_FILE_NOT_FOUND";
    if (status == ERROR_PATH_NOT_FOUND) return "ERROR_PATH_NOT_FOUND";
    if (status == ERROR_TOO_MANY_OPEN_FILES) return "ERROR_TOO_MANY_OPEN_FILES";
    if (status == ERROR_ACCESS_DENIED) return "ERROR_ACCESS_DENIED";
    if (status == ERROR_INVALID_HANDLE) return "ERROR_INVALID_HANDLE";
    return "UNKNOWN ERROR";
}

inline void print_error_message(LSTATUS status, const char* function_name, std::string optional_message = "") {
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    std::cerr << function_name << " failed with " << win_api_error_str(status) << ": "
              << std::string(static_cast<LPTSTR>(lpMsgBuf));
    if (optional_message != "") {
        std::cerr << " | " << optional_message;
    }
    std::cerr << "\n";
    LocalFree(lpMsgBuf);
}

inline std::string override_base_path(uint32_t random_base_path) {
    return std::string("SOFTWARE\\LoaderRegressionTests_") + std::to_string(random_base_path);
}

inline std::string get_override_path(HKEY root_key, uint32_t random_base_path) {
    std::string override_path = override_base_path(random_base_path);

    if (root_key == HKEY_CURRENT_USER) {
        override_path += "\\HKCU";
    } else if (root_key == HKEY_LOCAL_MACHINE) {
        override_path += "\\HKLM";
    }
    return override_path;
}

inline HKEY create_key(HKEY key_root, const char* key_path) {
    DWORD dDisposition{};
    HKEY key{};
    LSTATUS out =
        RegCreateKeyExA(key_root, key_path, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dDisposition);
    if (out != ERROR_SUCCESS) std::cerr << win_api_error_str(out) << " failed to create key " << key << " at " << key_path << "\n";
    return key;
}

inline void close_key(HKEY& key) {
    LSTATUS out = RegCloseKey(key);
    if (out != ERROR_SUCCESS) std::cerr << win_api_error_str(out) << " failed to close key " << key << "\n";
}

inline void override_key(HKEY root_key, uint32_t random_base_path) {
    DWORD dDisposition{};
    LSTATUS out;

    auto override_path = get_override_path(root_key, random_base_path);
    HKEY override_key;
    out = RegCreateKeyExA(HKEY_CURRENT_USER, override_path.c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                          &override_key, &dDisposition);
    if (out != ERROR_SUCCESS)
        std::cerr << win_api_error_str(out) << " failed to create key " << override_key << " with path " << override_path << "\n";

    out = RegOverridePredefKey(root_key, override_key);
    if (out != ERROR_SUCCESS) std::cerr << win_api_error_str(out) << " failed to override key " << override_key << "\n";

    close_key(override_key);
}
inline void revert_override(HKEY root_key, uint32_t random_base_path) {
    LSTATUS out = RegOverridePredefKey(root_key, NULL);
    if (out != ERROR_SUCCESS) std::cerr << win_api_error_str(out) << " failed to revert override key " << root_key << "\n";

    auto override_path = get_override_path(root_key, random_base_path);
    out = RegDeleteTreeA(HKEY_CURRENT_USER, override_path.c_str());
    if (out != ERROR_SUCCESS) print_error_message(out, "RegDeleteTreeA", std::string("Key") + override_path);
}

struct KeyWrapper {
    explicit KeyWrapper(HKEY key) noexcept : key(key) {}
    explicit KeyWrapper(HKEY key_root, const char* key_path) noexcept { key = create_key(key_root, key_path); }
    ~KeyWrapper() noexcept { close_key(key); }
    explicit KeyWrapper(KeyWrapper const&) = delete;
    KeyWrapper& operator=(KeyWrapper const&) = delete;

    HKEY get() const noexcept { return key; }
    operator HKEY() { return key; }
    operator HKEY() const { return key; }
    operator HKEY&() { return key; }
    operator HKEY const &() const { return key; }

    HKEY key{};
};

inline void add_key_value(HKEY const& key, fs::path const& manifest_path, bool enabled = true) {
    DWORD value = enabled ? 0 : 1;
    LSTATUS out = RegSetValueEx(key, manifest_path.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    if (out != ERROR_SUCCESS) std::cerr << win_api_error_str(out) << " failed to set key value for " << manifest_path.str() << "\n";
}

inline void remove_key_value(HKEY const& key, fs::path const& manifest_path) {
    LSTATUS out = RegDeleteValueA(key, manifest_path.c_str());
    if (out != ERROR_SUCCESS)
        std::cerr << win_api_error_str(out) << " failed to delete key value for " << manifest_path.str() << "\n";
}

inline void clear_key_values(HKEY const& key) {
    DWORD dwNumValues, dwValueNameLen;

    LSTATUS out = RegQueryInfoKey(key, 0, 0, 0, 0, 0, 0, &dwNumValues, &dwValueNameLen, 0, 0, 0);
    if (out != ERROR_SUCCESS) {
        std::cerr << win_api_error_str(out) << "couldn't query enum " << key << " values\n";
        return;
    }
    std::string tchValName(dwValueNameLen + 1, '\0');
    for (DWORD i = 0; i < dwNumValues; i++) {
        DWORD length = tchValName.size();
        LPSTR lpstr = &tchValName[0];
        out = RegEnumValue(key, i, lpstr, &length, 0, 0, 0, 0);
        if (out != ERROR_SUCCESS) {
            std::cerr << win_api_error_str(out) << "couldn't get enum value " << tchValName << "\n";
            return;
        }
    }
}

inline void PlatformShim::setup_override(DebugMode debug_mode) {
    std::random_device rd;
    std::ranlux48 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 2000000);
    random_base_path = dist(gen);
    override_key(HKEY_LOCAL_MACHINE, random_base_path);
    override_key(HKEY_CURRENT_USER, random_base_path);
}
inline void PlatformShim::clear_override(DebugMode debug_mode) {
    if (debug_mode != DebugMode::no_delete) {
        revert_override(HKEY_CURRENT_USER, random_base_path);
        revert_override(HKEY_LOCAL_MACHINE, random_base_path);

        LSTATUS out = RegDeleteKeyA(HKEY_CURRENT_USER, override_base_path(random_base_path).c_str());
        if (out != ERROR_SUCCESS)
            print_error_message(out, "RegDeleteKeyA", std::string("Key") + override_base_path(random_base_path).c_str());
    }
}
inline void PlatformShim::reset(DebugMode debug_mode) {
    KeyWrapper implicit_key{HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers"};
    KeyWrapper explicit_key{HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers"};
    KeyWrapper driver_key{HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\Drivers"};
    if (debug_mode != DebugMode::no_delete) {
        clear_key_values(implicit_key);
        clear_key_values(explicit_key);
        clear_key_values(driver_key);
    }
}

inline void PlatformShim::set_path(ManifestCategory category, fs::path const& path) {}

inline void PlatformShim::add_manifest(ManifestCategory category, fs::path const& path) {
    std::string reg_path = std::string("SOFTWARE\\Khronos\\Vulkan\\") + to_str(category);
    KeyWrapper key{HKEY_LOCAL_MACHINE, reg_path.c_str()};
    add_key_value(key, path);
    if (category == ManifestCategory::Driver) {
        drivers.push_back(path.str());
    }
}

inline void PlatformShim::add_dxgi_manifest(DXGIDriver const& driver) { dxgi_drivers.push_back(driver); }
inline void PlatformShim::add_dxgi_adapter_desc1(DXGI_ADAPTER_DESC1 desc1) { dxgi_adapter_desc1s.push_back(desc1); }

inline void PlatformShim::add_D3DKMT_ADAPTERINFO(D3DKMT_Adapter adapter) { d3dkmt_adapters.push_back(adapter); }

inline HKEY GetRegistryKey() { return HKEY{}; }

inline void PlatformShim::redirect_category(fs::path const& new_path, ManifestCategory search_category) {
    // create_key(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0001");
    // create_key(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0002");
    switch (search_category) {
        case (ManifestCategory::Implicit):
            create_key(HKEY_CURRENT_USER, "SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Khronos\\Vulkan\\ImplicitLayers");
            break;
        case (ManifestCategory::Explicit):
            create_key(HKEY_CURRENT_USER, "SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers");
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers");
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Khronos\\Vulkan\\ExplicitLayers");
            break;
        case (ManifestCategory::Driver):
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Khronos\\Vulkan\\Drivers");
            create_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Khronos\\Vulkan\\Drivers");
            break;
    }
}

#elif defined(__linux__) || defined(__APPLE__)

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

inline std::string to_str(ManifestCategory category) {
    if (category == ManifestCategory::Implicit) return "implicit_layer.d";
    if (category == ManifestCategory::Explicit)
        return "explicit_layer.d";
    else
        return "icd.d";
}

inline void PlatformShim::setup_override(DebugMode debug_mode) {}
inline void PlatformShim::clear_override(DebugMode debug_mode) {}
inline void PlatformShim::reset(DebugMode debug_mode) { redirection_map.clear(); }

inline void PlatformShim::add(fs::path const& path, fs::path const& new_path) { redirection_map[path.str()] = new_path; }
inline void PlatformShim::remove(fs::path const& path) { redirection_map.erase(path.str()); }
inline bool PlatformShim::is_fake_path(fs::path const& path) { return redirection_map.count(path.str()) > 0; }
inline fs::path const& PlatformShim::get_fake_path(fs::path const& path) { return redirection_map.at(path.str()); }

// void PlatformShim::set_implicit_layer_path(ManifestCategory category, fs::path const& path) {
//     add(fs::path("/usr/local/etc/vulkan/implicit_layer.d"), path);

//     add(fs::path("/usr/local/etc/vulkan/explicit_layer.d"), path);
//     add(fs::path("/usr/local/etc/vulkan/icd.d"), path);
// }

inline void PlatformShim::add_manifest(ManifestCategory category, fs::path const& path) {}

inline void PlatformShim::redirect_category(fs::path const& new_path, ManifestCategory category) {
    // default paths
    add(fs::path("/usr/local/etc/vulkan") / to_str(category), new_path);
    add(fs::path("/usr/local/share/vulkan") / to_str(category), new_path);
    add(fs::path("/etc/vulkan") / to_str(category), new_path);
    add(fs::path("/usr/share/vulkan") / to_str(category), new_path);

    std::vector<std::string> xdg_paths;
    std::string xdg_data_dirs_var = get_env_var("XDG_DATA_DIRS");
    if (xdg_data_dirs_var.size() > 0) {
        xdg_paths = parse_env_var_list(xdg_data_dirs_var);
    }
    std::string xdg_config_dirs_var = get_env_var("XDG_CONFIG_DIRS");
    if (xdg_config_dirs_var.size() > 0) {
        auto paths = parse_env_var_list(xdg_config_dirs_var);
        xdg_paths.insert(xdg_paths.begin(), paths.begin(), paths.end());
    }
    for (auto& path : xdg_paths) {
        add(fs::path(path) / "vulkan" / to_str(category), new_path);
    }

    std::string home = get_env_var("HOME");
    if (home.size() == 0) return;
    add(fs::path(home) / ".local/share/vulkan" / to_str(category), new_path);
}

inline void PlatformShim::set_path(ManifestCategory category, fs::path const& path) {
    if (category == ManifestCategory::Implicit) add(fs::path("/usr/local/etc/vulkan/implicit_layer.d"), path);
    if (category == ManifestCategory::Explicit) add(fs::path("/usr/local/etc/vulkan/explicit_layer.d"), path);
    if (category == ManifestCategory::Driver) add(fs::path("/usr/local/etc/vulkan/icd.d"), path);
}

#endif
