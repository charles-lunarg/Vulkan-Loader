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

set(CMAKE_CXX_STANDARD 11)

add_library(mock_dependencies INTERFACE)

target_compile_options(mock_dependencies INTERFACE
    $<$<PLATFORM_ID:WIN32>:${MSVC_LOADER_COMPILE_OPTIONS},/permissive->)

target_compile_definitions(mock_dependencies INTERFACE
# Workaround for TR1 deprecation in Visual Studio 15.5 until Google Test is updated
    $<$<PLATFORM_ID:WIN32>:-DWIN32_LEAN_AND_MEAN,-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING>
    VK_NO_PROTOTYPES)

target_link_libraries(mock_dependencies INTERFACE Vulkan::Headers vulkan_platform_enables testing_framework_util)
target_include_directories(mock_dependencies INTERFACE ${PROJECT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/framework ${CMAKE_BINARY_DIR}/tests/framework)

set(MOCK_DRIVER_SOURCES mock_driver.cpp mock_driver.h)

# mock driver without any config macros defined
add_library(mock_driver_export_none SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_export_none PRIVATE mock_dependencies)

add_library(mock_driver_export_icd_gipa SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_export_icd_gipa PRIVATE mock_dependencies)
target_compile_definitions(mock_driver_export_icd_gipa PRIVATE MOCK_DRIVER_EXPORT_ICD_GIPA=1)

add_library(mock_driver_export_negotiate_interface_version SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_export_negotiate_interface_version PRIVATE mock_dependencies)
target_compile_definitions(mock_driver_export_negotiate_interface_version PRIVATE MOCK_DRIVER_EXPORT_NEGOTIATE_INTERFACE_VERSION=1)

set(MOCK_DRIVER_VERSION_2_DEFINES MOCK_DRIVER_EXPORT_NEGOTIATE_INTERFACE_VERSION=1 MOCK_DRIVER_EXPORT_ICD_GIPA=1)

add_library(mock_driver_version_2 SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_version_2 PRIVATE mock_dependencies)
target_compile_definitions(mock_driver_version_2 PRIVATE ${MOCK_DRIVER_VERSION_2_DEFINES})

add_library(mock_driver_version_2_export_icd_enumerate_adapter_physical_devices SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_version_2_export_icd_enumerate_adapter_physical_devices PRIVATE mock_dependencies)
target_compile_definitions(mock_driver_version_2_export_icd_enumerate_adapter_physical_devices PRIVATE MOCK_DRIVER_EXPORT_ICD_ENUMERATE_ADAPTER_PHYSICAL_DEVICES=1 ${MOCK_DRIVER_VERSION_2_DEFINES})

add_library(mock_driver_version_2_export_icd_gpdpa SHARED ${MOCK_DRIVER_SOURCES})
target_link_libraries(mock_driver_version_2_export_icd_gpdpa PRIVATE mock_dependencies)
target_compile_definitions(mock_driver_version_2_export_icd_gpdpa PRIVATE MOCK_DRIVER_EXPORT_ICD_GPDPA=1 ${MOCK_DRIVER_VERSION_2_DEFINES})

# get driver location folder
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/driver_defs_$<CONFIG>.h"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/driver_defs.h.in")
# copy driver_defs_$<CONFIG> to driver_defs.h
add_custom_command (
    COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" "${CMAKE_CURRENT_BINARY_DIR}/driver_defs_$<CONFIG>.h" "${CMAKE_CURRENT_BINARY_DIR}/driver_defs.h"
    VERBATIM
    PRE_BUILD
    DEPENDS  "${CMAKE_CURRENT_BINARY_DIR}/driver_defs_$<CONFIG>.h"
    OUTPUT   "${CMAKE_CURRENT_BINARY_DIR}/driver_defs.h"
    COMMENT  "creating driver_defs.h file ({event: PRE_BUILD}, {filename: driver_defs.h})"
)
add_custom_target (driver_definitions_generated_output DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/driver_defs.h")

add_library(mock_driver_definitions INTERFACE)
target_include_directories(mock_driver_definitions INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(mock_driver_definitions driver_definitions_generated_output
    mock_driver_export_none
    mock_driver_export_icd_gipa 
    mock_driver_export_negotiate_interface_version 
    mock_driver_version_2 
    mock_driver_version_2_export_icd_enumerate_adapter_physical_devices 
    mock_driver_version_2_export_icd_gpdpa)

