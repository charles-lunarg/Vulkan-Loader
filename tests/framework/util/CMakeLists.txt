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
target_link_libraries(testing_framework_common PUBLIC Vulkan::Headers testing_framework_base_target)
if(UNIX)
    target_link_libraries(testing_framework_common PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(testing_framework_util STATIC test_util.cpp test_util.h)
target_link_libraries(testing_framework_util PRIVATE testing_framework_common PUBLIC testing_framework_base_target)

# Function which can process a C/C++ header file that contains CMake Generator expressions in it
# Arguments:
# CONFIG_FILE_NAME: header to run through cmake. Only list the stem, no .h or .h.should be present
# OUTPUT_TARGET_NAME: target name that gets created and allows other targets to depend on this generated header
function(loader_framework_config_helper CONFIG_FILE_NAME OUTPUT_TARGET_NAME)
    string(CONCAT SRC_DIR_INPUT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/" ${CONFIG_FILE_NAME} ".h.in")
    string(CONCAT BIN_DIR_INTERMEDIATE_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/" ${CONFIG_FILE_NAME} "_$<CONFIG>.h")
    string(CONCAT BIN_DIR_OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/" ${CONFIG_FILE_NAME} ".h")
    string(CONCAT CUSTOM_COMMAND_COMMENT "creating " ${CONFIG_FILE_NAME} ".h file ({event: PRE_BUILD}, {filename: " ${CONFIG_FILE_NAME} ".h })")

    # setup CONFIG_FILE_NAME.h
    file(GENERATE OUTPUT ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER} INPUT ${SRC_DIR_INPUT_CONFIG_FILE})

    # copy CONFIG_FILE_NAME_$<CONFIG> to the loader build directory
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER} ${BIN_DIR_OUTPUT_HEADER}
        VERBATIM
        PRE_BUILD
        DEPENDS  ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER}
        OUTPUT   ${BIN_DIR_OUTPUT_HEADER}
        COMMENT  "creating vulkan_library_path.h file ({event: PRE_BUILD}, {filename: vulkan_library_path.h})"
        )

    add_custom_target (${OUTPUT_TARGET_NAME} DEPENDS ${BIN_DIR_OUTPUT_HEADER})
endfunction()

loader_framework_config_helper(vulkan_library_path generated_vulkan_library_path_header)

target_include_directories(testing_framework_util PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(testing_framework_util generated_vulkan_library_path_header)
