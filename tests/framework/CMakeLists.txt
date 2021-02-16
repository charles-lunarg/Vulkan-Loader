# ~~~
# Copyright (c) 2021 Valve Corporation
# Copyright (c) 2021 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ~~~

add_library(testing_framework_base_target INTERFACE)
target_compile_options(testing_framework_base_target INTERFACE
    $<$<PLATFORM_ID:WIN32>:-DVK_USE_PLATFORM_WIN32_KHR>
    $<$<PLATFORM_ID:ANDROID>:-DVK_USE_PLATFORM_ANDROID_KHR>
    $<$<PLATFORM_ID:APPLE>:-DVK_USE_PLATFORM_MACOS_MVK>
    $<$<AND:$<PLATFORM_ID:UNIX>,$<NOT:$<PLATFORM_ID:APPLE>>>:
        $<$<BUILD_WSI_XCB_SUPPORT>:-DVK_USE_PLATFORM_XCB_KHR>
        $<$<BUILD_WSI_XLIB_SUPPORT>:-DVK_USE_PLATFORM_XLIB_KHR>
        $<$<BUILD_WSI_WAYLAND_SUPPORT>:-DVK_USE_PLATFORM_WAYLAND_KHR>>
    )
if(UNIX AND NOT APPLE)
    target_compile_options(testing_framework_base_target INTERFACE -fPIC)
endif()
target_include_directories(testing_framework_base_target INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(util)
add_subdirectory(shim)

add_subdirectory(icd)
add_subdirectory(layer)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/framework_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/framework_config.h")

add_library(test-environment STATIC test_environment.cpp test_environment.h)
target_link_libraries(test-environment
    PUBLIC test_icd_definitions test_layer_definitions gtest gtest_main
    testing_framework_common testing_framework_util shim-library)
target_include_directories(test-environment PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework ${CMAKE_BINARY_DIR}/tests/framework ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(test-environment PUBLIC "GTEST_LINKED_AS_SHARED_LIBRARY=1")

add_library(testing_dependencies INTERFACE)

# if (UNIX)
#     target_compile_options(testing_dependencies INTERFACE -fsanitize=address)
#     target_link_options(testing_dependencies INTERFACE -fsanitize=address)
# endif (UNIX)

target_include_directories(testing_dependencies INTERFACE ${CMAKE_SOURCE_DIR}/tests/framework ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(testing_dependencies INTERFACE $<$<PLATFORM_ID:WIN32>:${MSVC_LOADER_COMPILE_OPTIONS},/permissive->)
target_compile_definitions(testing_dependencies INTERFACE
# Workaround for TR1 deprecation in Visual Studio 15.5 until Google Test is updated
    $<$<PLATFORM_ID:WIN32>:-DWIN32_LEAN_AND_MEAN,-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING>)

target_compile_definitions(testing_dependencies INTERFACE "GTEST_LINKED_AS_SHARED_LIBRARY=1")
target_link_libraries(testing_dependencies INTERFACE Vulkan::Headers testing_framework_base_target test-environment)

add_executable(loader_versions_tests loader_version_tests.cpp)
target_link_libraries(loader_versions_tests PRIVATE testing_dependencies)

add_executable(loader_regression_tests loader_testing_framework.cpp loader_wsi_tests.cpp loader_testing_main.cpp )
target_link_libraries(loader_regression_tests PRIVATE testing_dependencies)

# Copy loader and googletest (gtest) libs to test dir so the test executable can find them.
if(WIN32)
    if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/external/googletest/googletest/$<CONFIG>/gtest_main$<$<CONFIG:Debug>:d>.dll
                            GTEST_COPY_SRC1)
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/external/googletest/googletest/$<CONFIG>/gtest$<$<CONFIG:Debug>:d>.dll
                            GTEST_COPY_SRC2)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG> GTEST_COPY_DEST)
    else()
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/external/googletest/googletest/gtest_main$<$<CONFIG:Debug>:d>.dll GTEST_COPY_SRC1)
        file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR}/external/googletest/googletest/gtest$<$<CONFIG:Debug>:d>.dll GTEST_COPY_SRC2)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} GTEST_COPY_DEST)
    endif()
    add_custom_command(TARGET loader_regression_tests POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_COPY_SRC1} ${GTEST_COPY_DEST}
                       COMMAND ${CMAKE_COMMAND} -E copy ${GTEST_COPY_SRC2} ${GTEST_COPY_DEST})
    # Copy the loader shared lib (if built) to the test application directory so the test app finds it.
    if(TARGET vulkan)
        add_custom_command(TARGET loader_regression_tests POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vulkan> $<TARGET_FILE_DIR:loader_regression_tests>)
    endif()
endif()
