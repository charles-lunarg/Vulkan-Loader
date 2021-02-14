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

class RegressionTests : public ::testing::Test {
   protected:
    virtual void SetUp() {
        env = std::unique_ptr<SingleDriverShim>(new SingleDriverShim(TestICDDetails(TEST_ICD_PATH_VERSION_2)));
    }

    virtual void TearDown() { env.reset(); }
    std::unique_ptr<SingleDriverShim> env;
};

#if defined(WIN32)

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProcA(hWnd, uMsg, wParam, lParam); }

TEST_F(RegressionTests, CreateSurfaceWin32) {
    auto& driver = env->get_test_icd();
    driver.SetDriverAPIVersion(VK_MAKE_VERSION(1, 0, 0));
    driver.SetMinICDInterfaceVersion(5);
    driver.AddInstanceExtension(Extension(VK_KHR_SURFACE_EXTENSION_NAME));
    driver.AddInstanceExtension(Extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME));
    driver.enable_icd_wsi = true;

    InstWrapper inst{env->vulkan_functions};
    auto inst_create_info = driver.GetVkInstanceCreateInfo();
    ASSERT_EQ(CreateInst(inst, inst_create_info), VK_SUCCESS);

    WNDCLASSEX win_class;
    const char* app_short_name = "loader_surface_test";
    HINSTANCE h_instance = GetModuleHandle(nullptr);  // Windows Instance
    HWND h_wnd = nullptr;                                       // window handle

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = h_instance;
    win_class.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = nullptr;
    win_class.lpszClassName = app_short_name;
    win_class.hIconSm = LoadIconA(nullptr, IDI_WINLOGO);

    // Register window class:
    EXPECT_TRUE(RegisterClassExA(&win_class) != NULL);

    // Create window with the registered class:
    RECT wr = {0, 0, 100, 100};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    h_wnd = CreateWindowExA(0,
                            app_short_name,  // class name
                            app_short_name,  // app name
                            // WS_VISIBLE | WS_SYSMENU |
                            WS_OVERLAPPEDWINDOW,  // window style
                            100, 100,             // x/y coords
                            wr.right - wr.left,   // width
                            wr.bottom - wr.top,   // height
                            nullptr,              // handle to parent
                            nullptr,              // handle to menu
                            h_instance,           // hInstance
                            nullptr);             // no extra parameters
    EXPECT_TRUE(h_wnd != nullptr);

    VkSurfaceKHR surface{};
    VkWin32SurfaceCreateInfoKHR surf_create_info{};
    surf_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surf_create_info.hwnd = h_wnd;
    surf_create_info.hinstance = h_instance;
    VkResult res = env->vulkan_functions.fp_vkCreateWin32SurfaceKHR(inst, &surf_create_info, nullptr, &surface);
    ASSERT_EQ(res, VK_SUCCESS);
    ASSERT_TRUE(surface != nullptr);
//    ASSERT_EQ(driver.is_using_icd_wsi, UsingICDProvidedWSI::not_using);
}

#endif
