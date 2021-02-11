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

if (WIN32)

add_library(shim-library SHARED shim.h windows_shim.cpp)
target_link_libraries(shim-library PRIVATE detours cfgmgr32 testing_framework_common)
target_include_directories(shim-library PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework ${CMAKE_CURRENT_BINARY_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/loader)

elseif(UNIX)
add_library(shim-library SHARED shim.h linux_shim.cpp)
target_link_libraries(shim-library PRIVATE testing_framework_common)
target_include_directories(shim-library PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

elseif(APPLE)

endif()

# get shim location folder
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shim_config_$<CONFIG>.h"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shim_config.h.in")
# copy shim_config_$<CONFIG> to shim_config.h
add_custom_command (
    COMMAND ${CMAKE_COMMAND} "-E" "copy_if_different" "${CMAKE_CURRENT_BINARY_DIR}/shim_config_$<CONFIG>.h" "${CMAKE_CURRENT_BINARY_DIR}/shim_config.h"
    VERBATIM
    PRE_BUILD
    DEPENDS  "${CMAKE_CURRENT_BINARY_DIR}/shim_config_$<CONFIG>.h"
    OUTPUT   "${CMAKE_CURRENT_BINARY_DIR}/shim_config.h"
    COMMENT  "creating shim_config.h file ({event: PRE_BUILD}, {filename: shim_config.h})"
)
add_custom_target (shim_config_generated_output DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/shim_config.h")

add_dependencies(shim-library shim_config_generated_output)

