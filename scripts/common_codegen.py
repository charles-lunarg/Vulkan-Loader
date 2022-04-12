#!/usr/bin/python3 -i
#
# Copyright (c) 2015-2017, 2019-2021 The Khronos Group Inc.
# Copyright (c) 2015-2017, 2019-2021 Valve Corporation
# Copyright (c) 2015-2017, 2019-2021 LunarG, Inc.
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
#
# Author: Mark Lobodzinski <mark@lunarg.com>

import os,re

# Copyright text prefixing all headers (list of strings).
prefixStrings = [
    '/*',
    '** Copyright (c) 2015-2017, 2019-2022 The Khronos Group Inc.',
    '** Copyright (c) 2015-2017, 2019-2022 Valve Corporation',
    '** Copyright (c) 2015-2017, 2019-2022 LunarG, Inc.',
    '** Copyright (c) 2015-2017, 2019-2022 Google Inc.',
    '**',
    '** Licensed under the Apache License, Version 2.0 (the "License");',
    '** you may not use this file except in compliance with the License.',
    '** You may obtain a copy of the License at',
    '**',
    '**     http://www.apache.org/licenses/LICENSE-2.0',
    '**',
    '** Unless required by applicable law or agreed to in writing, software',
    '** distributed under the License is distributed on an "AS IS" BASIS,',
    '** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.',
    '** See the License for the specific language governing permissions and',
    '** limitations under the License.',
    '*/',
    ''
]


platform_dict = {
    'android' : 'VK_USE_PLATFORM_ANDROID_KHR',
    'fuchsia' : 'VK_USE_PLATFORM_FUCHSIA',
    'ggp': 'VK_USE_PLATFORM_GGP',
    'ios' : 'VK_USE_PLATFORM_IOS_MVK',
    'macos' : 'VK_USE_PLATFORM_MACOS_MVK',
    'metal' : 'VK_USE_PLATFORM_METAL_EXT',
    'vi' : 'VK_USE_PLATFORM_VI_NN',
    'wayland' : 'VK_USE_PLATFORM_WAYLAND_KHR',
    'win32' : 'VK_USE_PLATFORM_WIN32_KHR',
    'xcb' : 'VK_USE_PLATFORM_XCB_KHR',
    'xlib' : 'VK_USE_PLATFORM_XLIB_KHR',
    'directfb' : 'VK_USE_PLATFORM_DIRECTFB_EXT',
    'xlib_xrandr' : 'VK_USE_PLATFORM_XLIB_XRANDR_EXT',
    'provisional' : 'VK_ENABLE_BETA_EXTENSIONS',
    'screen' : 'VK_USE_PLATFORM_SCREEN_QNX',
}

#
# Return appropriate feature protect string from 'platform' tag on feature
def GetFeatureProtect(interface):
    """Get platform protection string"""
    platform = interface.get('platform')
    protect = None
    if platform is not None:
        protect = platform_dict[platform]
    return protect

# helper to define paths relative to the repo root
def repo_relative(path):
    return os.path.abspath(os.path.join(os.path.dirname(__file__), '..', path))

#
# Check if the parameter passed in is a static array
def paramIsStaticArray(param):
    is_static_array = 0
    paramname = param.find('name')
    if (paramname.tail is not None) and ('[' in paramname.tail):
        is_static_array = paramname.tail.count('[')
    return is_static_array

# Retrieve the value of the len tag
def getLen(param):
    result = None
    # Default to altlen when available to avoid LaTeX markup
    if 'altlen' in param.attrib:
        len = param.attrib.get('altlen')
    else:
        len = param.attrib.get('len')
    if len and len != 'null-terminated':
        # Only first level is supported for multidimensional arrays. Conveniently, this also strips the trailing
        # 'null-terminated' from arrays of strings
        len = len.split(',')[0]
        # Convert scope notation to pointer access
        result = str(len).replace('::', '->')
    elif paramIsStaticArray(param):
        # For static arrays get length from inside []
        array_match = re.search(r'\[(\d+)\]', param.find('name').tail)
        if array_match:
            result = array_match.group(1)
    return result

# Check if the parameter passed in is a pointer
def paramIsPointer(param):
    is_pointer = False
    for elem in param:
        if elem.tag == 'type' and elem.tail is not None and '*' in elem.tail:
            is_pointer = True
    return is_pointer

# Check if the parameter passed in is an array
def paramIsArray(param):
    is_array = False
    for elem in param:
        if elem.tail is not None and '[' in elem.tail and ']' in elem.tail:
            is_array = True
    return is_array

# Return the sizes of the arrays
def paramGetArraySizes(param):
    array_sizes = [1, 0]
    for elem in param:
        if elem.tail is not None and '[' in elem.tail and ']' in elem.tail:
            tmp_array_sizes = list(map(int, re.findall(r'\d+', elem.tail)))
            if len(tmp_array_sizes) >= 2:
                array_sizes[1] = tmp_array_sizes[1]
            if len(tmp_array_sizes) >= 1:
                array_sizes[0] = tmp_array_sizes[0]
    return array_sizes