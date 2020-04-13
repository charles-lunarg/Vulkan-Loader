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

// This needs to be defined first, or else we'll get redefinitions on NTSTATUS values
#ifdef _WIN32
#define UMDF_USING_NTSTATUS
#include <ntstatus.h>
#endif

#include "shim.h"

#include <cfgmgr32.h>
#include <initguid.h>
#include <devpkey.h>
#include <winternl.h>
#include <strsafe.h>

#define CINTERFACE
#include <dxgi1_6.h>
#include "adapters.h"

#include "detours.h"

#include "util/common.h"

static PlatformShim platform_shim;

extern "C" {

static LibraryWrapper gdi32_dll;

static PFN_LoaderEnumAdapters2 fpEnumAdapters2 = nullptr;
static PFN_LoaderQueryAdapterInfo fpQueryAdapterInfo = nullptr;

static CONFIGRET(WINAPI *REAL_CM_Get_Device_ID_List_SizeW)(PULONG pulLen, PCWSTR pszFilter,
                                                           ULONG ulFlags) = CM_Get_Device_ID_List_SizeW;
static CONFIGRET(WINAPI *REAL_CM_Get_Device_ID_ListW)(PCWSTR pszFilter, PZZWSTR Buffer, ULONG BufferLen,
                                                      ULONG ulFlags) = CM_Get_Device_ID_ListW;
long ShimEnumAdapters2(LoaderEnumAdapters2 *adapters) {
    // std::cerr << "shim EnumAdapters2\n";

    // NTSTATUS status = fpLoaderEnumAdapters2(adapters);

    // if (status == STATUS_SUCCESS && adapters->adapter_count > 0) {
    //     adapters->adapter_count = 0;
    // }

    // do nothing and just return success.
    return STATUS_SUCCESS;
}
NTSTATUS APIENTRY ShimQueryAdapterInfo(const LoaderQueryAdapterInfo *query_info) { return fpQueryAdapterInfo(query_info); }

// static const wchar_t *displayGUID = L"{4d36e968-e325-11ce-bfc1-08002be10318}";

CONFIGRET WINAPI SHIM_CM_Get_Device_ID_List_SizeW(PULONG pulLen, PCWSTR pszFilter, ULONG ulFlags) {
    // if (wcscmp(displayGUID, pszFilter) != 0) {
    //     return CM_Get_Device_ID_List_SizeW(pulLen, pszFilter, ulFlags);
    // }
    *pulLen = 1;
    return CR_SUCCESS;
}
CONFIGRET WINAPI SHIM_CM_Get_Device_ID_ListW(PCWSTR pszFilter, PZZWSTR Buffer, ULONG BufferLen, ULONG ulFlags) {
    // if (wcscmp(displayGUID, pszFilter) != 0) {
    //     return CM_Get_Device_ID_ListW(pszFilter, Buffer, BufferLen, ulFlags);
    // }
    if (BufferLen == 0) {
        return CR_BUFFER_SMALL;
    }
    if (Buffer != NULL) {
        Buffer[0] = 0;
    }
    return CR_SUCCESS;
}

static LibraryWrapper dxgi_module;
typedef HRESULT(APIENTRY *PFN_CreateDXGIFactory1)(REFIID riid, void **ppFactory);

HRESULT(STDMETHODCALLTYPE *RealEnumAdapters1)
(IDXGIFactory1 *This,
 /* [in] */ UINT Adapter,
 /* [annotation][out] */
 _COM_Outptr_ IDXGIAdapter1 **ppAdapter);

HRESULT(STDMETHODCALLTYPE *RealEnumAdapterByGpuPreference)
(IDXGIFactory6 *This,
 /* [annotation] */
 _In_ UINT Adapter,
 /* [annotation] */
 _In_ DXGI_GPU_PREFERENCE GpuPreference,
 /* [annotation] */
 _In_ REFIID riid,
 /* [annotation] */
 _COM_Outptr_ void **ppvAdapter);

HRESULT(STDMETHODCALLTYPE *RealGetDesc1)
(IDXGIAdapter1 *This,
 /* [annotation][out] */
 _Out_ DXGI_ADAPTER_DESC1 *pDesc);

HRESULT STDMETHODCALLTYPE ShimEnumAdapters1(IDXGIFactory1 *This,
                                            /* [in] */ UINT Adapter,
                                            /* [annotation][out] */
                                            _COM_Outptr_ IDXGIAdapter1 **ppAdapter) {
    // std::cerr << "Shim EnumAdapters1\n";
    if (Adapter < platform_shim.driver_count()) {
        return RealEnumAdapters1(This, Adapter, ppAdapter);
    } else {
        return DXGI_ERROR_NOT_FOUND;
    }
}

HRESULT STDMETHODCALLTYPE ShimEnumAdapterByGpuPreference(IDXGIFactory6 *This, _In_ UINT Adapter,
                                                         _In_ DXGI_GPU_PREFERENCE GpuPreference, _In_ REFIID riid,
                                                         _COM_Outptr_ void **ppvAdapter) {
    // std::cerr << "Shim EnumAdapterByGpuPreference\n";
    if (Adapter < platform_shim.driver_count()) {
        return RealEnumAdapterByGpuPreference(This, Adapter, GpuPreference, riid, ppvAdapter);
    } else {
        return DXGI_ERROR_NOT_FOUND;
    }
}

HRESULT STDMETHODCALLTYPE ShimGetDesc1(IDXGIAdapter1 *This,
                                       /* [annotation][out] */
                                       _Out_ DXGI_ADAPTER_DESC1 *pDesc) {
    // std::cout << "Intercepted GetDesc1\n";
    return RealGetDesc1(This, pDesc);
}

void WINAPI DetourEnumAdapterFunctions() {
    if (RealEnumAdapters1 != nullptr && RealEnumAdapterByGpuPreference != nullptr) {
        return;
    }
    PFN_CreateDXGIFactory1 fpCreateDXGIFactory1;
    if (!dxgi_module) {
        TCHAR systemPath[MAX_PATH] = "";
        GetSystemDirectory(systemPath, MAX_PATH);
        StringCchCat(systemPath, MAX_PATH, TEXT("\\dxgi.dll"));
        dxgi_module = LibraryWrapper(systemPath);
        fpCreateDXGIFactory1 = dxgi_module.get_symbol<PFN_CreateDXGIFactory1>("CreateDXGIFactory1");
        if (fpCreateDXGIFactory1 == nullptr) {
            std::cerr << "Failed to load CreateDXGIFactory1\n";
        }
    }
    IDXGIFactory1 *dxgi_factory_1 = nullptr;
    IDXGIFactory6 *dxgi_factory_6 = nullptr;
    IDXGIAdapter1 *adapter = nullptr;

    HRESULT res = fpCreateDXGIFactory1(IID_IDXGIFactory1, (void **)&dxgi_factory_1);
    if (res != S_OK) {
        std::cerr << "Failed to create DXGIFactory1\n";
    } else {
        RealEnumAdapters1 = dxgi_factory_1->lpVtbl->EnumAdapters1;
    }

    res = fpCreateDXGIFactory1(IID_IDXGIFactory6, (void **)&dxgi_factory_6);
    if (res != S_OK) {
        std::cerr << "Failed to create DXGIFactory6\n";
    } else {
        RealEnumAdapterByGpuPreference = dxgi_factory_6->lpVtbl->EnumAdapterByGpuPreference;
    }

    if (!dxgi_factory_1) {
        res = dxgi_factory_1->lpVtbl->EnumAdapters1(dxgi_factory_1, 0, &adapter);
        if (res == DXGI_ERROR_NOT_FOUND) {
            std::cerr << "Couldn't get the 0th adapter\n";
        } else if (res == S_OK) {
            RealGetDesc1 = adapter->lpVtbl->GetDesc1;
        }
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (dxgi_factory_1 != nullptr) {
        DetourAttach(&(PVOID &)RealEnumAdapters1, ShimEnumAdapters1);
    }
    if (dxgi_factory_6 != nullptr) {
        DetourAttach(&(PVOID &)RealEnumAdapterByGpuPreference, ShimEnumAdapterByGpuPreference);
    }
    // Doesn't work currently
    // if (!adapter) {
    //     DetourAttach(&(PVOID &)RealGetDesc1, ShimGetDesc1);
    // }

    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        std::cerr << "simple" << DETOURS_STRINGIFY(DETOURS_BITS) << ".dll:"
                  << " Error detouring function(): " << error << "\n";
    }
    if (dxgi_factory_1 != NULL) {
        dxgi_factory_1->lpVtbl->Release(dxgi_factory_1);
    }
    if (dxgi_factory_6 != NULL) {
        dxgi_factory_6->lpVtbl->Release(dxgi_factory_6);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        if (!gdi32_dll) {
            gdi32_dll = LibraryWrapper("gdi32.dll");
            fpEnumAdapters2 = gdi32_dll.get_symbol<PFN_LoaderEnumAdapters2>("D3DKMTEnumAdapters2");
            if (fpEnumAdapters2 == nullptr) {
                std::cerr << "Failed to load D3DKMTEnumAdapters2\n";
                return TRUE;
            }
            fpQueryAdapterInfo = gdi32_dll.get_symbol<PFN_LoaderQueryAdapterInfo>("D3DKMTQueryAdapterInfo");
            if (fpQueryAdapterInfo == nullptr) {
                std::cerr << "Failed to load D3DKMTQueryAdapterInfo\n";
                return TRUE;
            }
        }

        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID &)fpEnumAdapters2, ShimEnumAdapters2);
        DetourAttach(&(PVOID &)fpQueryAdapterInfo, ShimQueryAdapterInfo);
        DetourAttach(&(PVOID &)REAL_CM_Get_Device_ID_List_SizeW, SHIM_CM_Get_Device_ID_List_SizeW);
        DetourAttach(&(PVOID &)REAL_CM_Get_Device_ID_ListW, SHIM_CM_Get_Device_ID_ListW);
        LONG error = DetourTransactionCommit();

        if (error != NO_ERROR) {
            std::cerr << "simple" << DETOURS_STRINGIFY(DETOURS_BITS) << ".dll:"
                      << " Error detouring function(): " << error << "\n";
        }
    } else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID &)fpEnumAdapters2, ShimEnumAdapters2);
        DetourDetach(&(PVOID &)fpQueryAdapterInfo, ShimQueryAdapterInfo);
        DetourDetach(&(PVOID &)REAL_CM_Get_Device_ID_List_SizeW, SHIM_CM_Get_Device_ID_List_SizeW);
        DetourDetach(&(PVOID &)REAL_CM_Get_Device_ID_ListW, SHIM_CM_Get_Device_ID_ListW);
        if (RealEnumAdapters1 != nullptr) {
            DetourDetach(&(PVOID &)RealEnumAdapters1, (PVOID)ShimEnumAdapters1);
        }
        if (RealEnumAdapterByGpuPreference != nullptr) {
            DetourDetach(&(PVOID &)RealEnumAdapterByGpuPreference, (PVOID)ShimEnumAdapterByGpuPreference);
        }
        DetourTransactionCommit();
    }
    return TRUE;
}
FRAMEWORK_EXPORT PlatformShim &get_platform_shim() {
    platform_shim = PlatformShim();
    DetourEnumAdapterFunctions();
    return platform_shim;
}
}