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
target_link_libraries(testing_framework_util PUBLIC testing_framework_common)

# Function which can process a C/C++ header file that contains CMake Generator expressions in it
# Arguments:
# CONFIG_FILE_NAME: header to run through cmake. Only list the stem, no .h or .h.should be present
# OUTPUT_TARGET_NAME: Target to create which other targets can depend on
# DEPENDANT_TARGET: Target name which depends on this config file beign present
function(loader_framework_config_helper CONFIG_FILE_NAME OUTPUT_TARGET_NAME)
    string(CONCAT SRC_DIR_INPUT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/" ${CONFIG_FILE_NAME} ".h.in")
    string(CONCAT BIN_DIR_INTERMEDIATE_CONFIG_HEADER "${CMAKE_CURRENT_BINARY_DIR}/" ${CONFIG_FILE_NAME} "_$<CONFIG>.h")
    string(CONCAT BIN_DIR_OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/" ${CONFIG_FILE_NAME} ".h")
    string(CONCAT OUTPUT_CUSTOM_TARGET ${CONFIG_FILE_NAME} "_custom_target")

    string(CONCAT CUSTOM_COMMAND_COMMENT "creating " ${CONFIG_FILE_NAME} ".h file ({event: PRE_BUILD}, {filename: " ${CONFIG_FILE_NAME} ".h })")

    # setup CONFIG_FILE_NAME.h
    file(GENERATE OUTPUT ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER} INPUT ${SRC_DIR_INPUT_CONFIG_FILE})

    # copy CONFIG_FILE_NAME_$<CONFIG> to the loader build directory
    add_custom_command(
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER} ${BIN_DIR_OUTPUT_HEADER}
        VERBATIM
        PRE_BUILD
        DEPENDS  ${BIN_DIR_INTERMEDIATE_CONFIG_HEADER}
        OUTPUT   ${BIN_DIR_OUTPUT_HEADER}
        COMMENT  ${CUSTOM_COMMAND_COMMENT}
        )
    add_custom_target (${OUTPUT_CUSTOM_TARGET} DEPENDS ${BIN_DIR_OUTPUT_HEADER})
    add_library(${OUTPUT_TARGET_NAME} INTERFACE)
    target_include_directories(${OUTPUT_TARGET_NAME} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies (${OUTPUT_TARGET_NAME} ${OUTPUT_CUSTOM_TARGET})
endfunction()

loader_framework_config_helper(vulkan_library_path vulkan_library_path_generated_target)
target_link_libraries(testing_framework_util PUBLIC vulkan_library_path_generated_target)
