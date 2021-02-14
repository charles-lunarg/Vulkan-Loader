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

add_library(testing_framework_common STATIC common.cpp common.h)
target_link_libraries(testing_framework_common PUBLIC Vulkan::Headers vulkan_platform_enables testing_framework_base_include_target)
if(UNIX)
    target_link_libraries(testing_framework_common PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(testing_framework_util STATIC test_util.cpp test_util.h)
target_link_libraries(testing_framework_util PRIVATE testing_framework_common PUBLIC testing_framework_base_include_target)

# setup vulkan_library_path.h
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path_$<CONFIG>.h"
INPUT "${CMAKE_CURRENT_SOURCE_DIR}/vulkan_library_path.h.in")

# copy vulkan_library_path_$<CONFIG> to the loader build directory
add_custom_command(
    COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path_$<CONFIG>.h" "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path.h"
    VERBATIM
    PRE_BUILD
    DEPENDS  "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path_$<CONFIG>.h"
    OUTPUT   "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path.h"
    COMMENT  "creating vulkan_library_path.h file ({event: PRE_BUILD}, {filename: vulkan_library_path.h})"
    )

add_custom_target (generated_vulkan_library_path_header DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vulkan_library_path.h")

target_include_directories(testing_framework_util PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(testing_framework_util generated_vulkan_library_path_header)
