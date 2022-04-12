#!/usr/bin/python3 -i
#
# Copyright (c) 2022 The Khronos Group Inc.
# Copyright (c) 2022 Valve Corporation
# Copyright (c) 2022 LunarG, Inc.
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
# Author: Mark Young <marky@lunarg.com>

import os,re,sys
import xml.etree.ElementTree as etree
from generator import *
from collections import namedtuple
from common_codegen import *

MANUAL_LAYER_DRIVER_ENTRYPOINTS = [
    'vkEnumerateInstanceExtensionProperties',
    'vkEnumerateInstanceLayerProperties',
    'vkEnumerateDeviceExtensionProperties',
    'vkEnumerateDeviceLayerProperties',
    'vkEnumerateInstanceVersion',
    'vkCreateInstance',
    'vkDestroyInstance',
    'vkCreateDevice',
    'vkDestroyDevice',
    'vkGetInstanceProcAddr',
    'vkGetDeviceProcAddr',
    'vkNegotiateLoaderLayerInterfaceVersion',
    'vkCreateDebugUtilsMessengerEXT',
]
MANUAL_DRIVER_ENTRYPOINTS = [
    'vkEnumeratePhysicalDevices',
    'vkEnumeratePhysicalDeviceGroups',
    'vkGetPhysicalDeviceProperties',
    'vkGetPhysicalDeviceProperties2',
]
EXTENSIONS_TO_SKIP_TESTING = [
    'VK_EXT_debug_utils'
]

# LoaderTestGeneratorOptions - subclass of GeneratorOptions.
class LoaderTestGeneratorOptions(GeneratorOptions):
    def __init__(self,
                 conventions = None,
                 filename = None,
                 directory = '.',
                 genpath = None,
                 apiname = None,
                 profile = None,
                 versions = '.*',
                 emitversions = '.*',
                 defaultExtensions = None,
                 addExtensions = None,
                 removeExtensions = None,
                 emitExtensions = None,
                 sortProcedure = regSortFeatures,
                 prefixText = "",
                 genFuncPointers = True,
                 protectFile = True,
                 protectFeature = True,
                 apicall = '',
                 apientry = '',
                 apientryp = '',
                 indentFuncProto = True,
                 indentFuncPointer = False,
                 alignFuncParam = 0,
                 expandEnumerants = True):
        GeneratorOptions.__init__(self,
                conventions = conventions,
                filename = filename,
                directory = directory,
                genpath = genpath,
                apiname = apiname,
                profile = profile,
                versions = versions,
                emitversions = emitversions,
                defaultExtensions = defaultExtensions,
                addExtensions = addExtensions,
                removeExtensions = removeExtensions,
                emitExtensions = emitExtensions,
                sortProcedure = sortProcedure)
        self.prefixText      = prefixText
        self.prefixText      = None
        self.apicall         = apicall
        self.apientry        = apientry
        self.apientryp       = apientryp
        self.alignFuncParam  = alignFuncParam
        self.expandEnumerants = expandEnumerants

# LoaderTestOutputGenerator - subclass of OutputGenerator.
class LoaderTestOutputGenerator(OutputGenerator):
    """Generate common trampoline and terminator functions based on incoming registry file data"""
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)

        self.max_major = 1
        self.max_minor = 0
        self.handles = {}
        self.handle_tree = []
        self.commands = {}
        self.extensions = {}
        self.test_variables = []
        self.struct_data = {}

    # Called once at the beginning of each run
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)

        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)

        # File Comment
        file_comment = '// *** THIS FILE IS GENERATED - DO NOT EDIT ***\n'
        file_comment += '// See loader_test_generator.py for modifications\n'
        write(file_comment, file=self.outFile)

        # Copyright Notice
        copyright =  '/*\n'
        copyright += ' * Copyright (c) 2022 The Khronos Group Inc.\n'
        copyright += ' * Copyright (c) 2022 Valve Corporation\n'
        copyright += ' * Copyright (c) 2022 LunarG, Inc.\n'
        copyright += ' *\n'
        copyright += ' * Licensed under the Apache License, Version 2.0 (the "License");\n'
        copyright += ' * you may not use this file except in compliance with the License.\n'
        copyright += ' * You may obtain a copy of the License at\n'
        copyright += ' *\n'
        copyright += ' *     http://www.apache.org/licenses/LICENSE-2.0\n'
        copyright += ' *\n'
        copyright += ' * Unless required by applicable law or agreed to in writing, software\n'
        copyright += ' * distributed under the License is distributed on an "AS IS" BASIS,\n'
        copyright += ' * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n'
        copyright += ' * See the License for the specific language governing permissions and\n'
        copyright += ' * limitations under the License.\n'
        copyright += ' *\n'
        copyright += ' * Author: Mark Young <marky@lunarg.com>\n'
        copyright += ' */\n'

        preamble = '// clang-format off\n\n'

        if self.genOpts.filename[-2:] == '.h':
            preamble += '#pragma once\n\n'

        if self.genOpts.filename == 'vk_test_entrypoint_core_tests.cpp':
            preamble += '#include "test_environment.h"\n'
            preamble += '#include "loader/generated/vk_dispatch_table_helper.h"\n'

        elif self.genOpts.filename == 'vk_test_entrypoint_extension_tests.cpp':
            preamble += '#include "test_environment.h"\n'
            preamble += '#include "loader/generated/vk_dispatch_table_helper.h"\n'

        elif self.genOpts.filename == 'vk_test_entrypoint_layer.h':
            preamble += '#include "test_util.h"\n'
            preamble += '#include "layer/layer_util.h"\n'
            preamble += '#include "loader/generated/vk_layer_dispatch_table.h"\n'

        elif self.genOpts.filename == 'vk_test_entrypoint_layer.cpp':
            preamble += '#include "vk_test_entrypoint_layer.h"\n'
            preamble += '#include "loader/generated/vk_dispatch_table_helper.h"\n'

        elif self.genOpts.filename == 'vk_test_entrypoint_driver.h':
            preamble += '#include "test_util.h"\n'
            preamble += '#include "layer/layer_util.h"\n'

        elif self.genOpts.filename == 'vk_test_entrypoint_driver.cpp':
            preamble += '#include "vk_test_entrypoint_driver.h"\n'

        write(copyright, file=self.outFile)
        write(preamble, file=self.outFile)

    # Called when done doing the processing of the XML data file
    def endFile(self):
        # Generate the handle tree data so we can determine what handles
        # are needed for what other handles
        self.GenerateHandleTreeData()

        file_data = ''

        if self.genOpts.filename == 'vk_test_entrypoint_core_tests.cpp':
            for count in range(0, 2):
                use_dispatch_table = True
                if count == 1:
                    use_dispatch_table = False
                for ext in self.extensions.values():
                    if 'VK_VERSION_' in ext.name:
                        file_data += self.GenerateCoreTest(ext, use_dispatch_table)
        elif self.genOpts.filename == 'vk_test_entrypoint_extension_tests.cpp':
            for ext in self.extensions.values():
                if ('VK_VERSION_' not in ext.name and
                    ext.name not in EXTENSIONS_TO_SKIP_TESTING and
                    len(ext.command_data) > 0):
                    file_data += self.GenerateExtensionTest(ext, True)
        elif self.genOpts.filename == 'vk_test_entrypoint_layer.h':
            file_data += self.GenerateLayerHeader()
        elif self.genOpts.filename == 'vk_test_entrypoint_layer.cpp':
            file_data += self.GenerateLayerSource()
        elif self.genOpts.filename == 'vk_test_entrypoint_driver.h':
            file_data += self.GenerateDriverHeader()
        elif self.genOpts.filename == 'vk_test_entrypoint_driver.cpp':
            file_data += self.GenerateDriverSource()

        file_data += '// clang-format on\n'

        write(file_data, file=self.outFile);

        # Finish processing in superclass
        OutputGenerator.endFile(self)


    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)

        self.featureExtraProtect = GetFeatureProtect(interface)

        enums = interface[0].findall('enum')
        name_definition = ''

        for item in enums:
            name_def = item.get('name')
            if 'EXTENSION_NAME' in name_def:
                name_definition = name_def

        type = interface.get('type')
        name = interface.get('name')

        if 'android' in name.lower():
            return

        if 'VK_VERSION_' in name:
            version_numbers = list(map(int, re.findall(r'\d+', name)))
            if version_numbers[0] > self.max_major:
                self.max_major = version_numbers[0]
            if version_numbers[1] > self.max_minor:
                self.max_minor = version_numbers[1]


        commands = {}
        command_req_ext = {}
        for require_element in interface.findall('require'):
            req_ext = require_element.get('extension')
            xml_commands = require_element.findall('command')
            if xml_commands:
                for xml_command in xml_commands:
                    command_name = xml_command.get('name')
                    skip = False
                    for ext in self.extensions.values():
                        cmd = ext.command_data.get(command_name)
                        if cmd is not None:
                            skip = True
                            break
                    if not skip:
                        required_ext_list = []
                        if not req_ext is None:
                            required_ext_list = req_ext.split(',')
                        commands[command_name] = required_ext_list
                        command_req_ext.update(dict.fromkeys(required_ext_list))

        requires = interface.get('requires')
        required_exts = dict.fromkeys(requires.split(',')) if requires is not None else {}
        required_exts.update(command_req_ext)

        self.extensions[name] = BasicExtensionData(
                name = name,
                type = type,
                protect = self.featureExtraProtect,
                define_name = name_definition,
                required_exts = required_exts,
                command_data = commands
            )


    # Called for each type -- if the type is a struct/union, grab the metadata
    def genType(self, typeinfo, name, alias):
        OutputGenerator.genType(self, typeinfo, name, alias)

        typeElem = typeinfo.elem
        category = typeElem.get('category')
        if category == 'handle' and name[0:2] == 'Vk':
            dispatchable = False
            if (typeElem.find('type') is not None and typeElem.find('type').text is not None and
                typeElem.find('type').text == 'VK_DEFINE_HANDLE'):
                dispatchable = True
            self.handles[name] = BasicHandleData(
                    name = name,
                    alias = alias,
                    is_dispatchable = dispatchable
                )
        elif (category == 'struct' or category == 'union'):
            self.genStruct(typeinfo, name, alias)

    # Generate a VkStructureType based on a structure typename
    def genVkStructureType(self, typename):
        # Add underscore between lowercase then uppercase
        value = re.sub('([a-z0-9])([A-Z])', r'\1_\2', typename)
        # Change to uppercase
        value = value.upper()
        # Add STRUCTURE_TYPE_
        return re.sub('VK_', 'VK_STRUCTURE_TYPE_', value)

    # Generate local ready-access data describing Vulkan structures and unions from the XML metadata
    def genStruct(self, typeinfo, typeName, alias):
        members = typeinfo.elem.findall('.//member')
        # Generate member info
        members_data = []
        for member in members:
            # Get the member's type and name
            type = member.find('type').text if member.find('type') is not None else ''
            name = member.find('name').text if member.find('name') is not None else ''
            cdecl = self.makeCParamDecl(member, 1)
            req_value = ''
            # Process VkStructureType
            if type == 'VkStructureType':
                # Extract the required struct type value from the comments
                # embedded in the original text defining the 'typeinfo' element
                rawXml = etree.tostring(typeinfo.elem).decode('ascii')
                result = re.search(r'VK_STRUCTURE_TYPE_\w+', rawXml)
                if result:
                    req_value = result.group(0)
                else:
                    req_value = self.genVkStructureType(typeName)

            members_data.append(
                StructMember(
                    type = type,
                    name = name,
                    req_value = req_value,
                    cdecl = cdecl))
        self.struct_data[typeName] = StructData(
            name = typeName,
            protect = self.featureExtraProtect,
            members = members_data)

    def paramIsHandle(self, param):
        if param[0:2] == 'Vk' and 'VkAlloc' not in param:
            for handle_name in self.handles.keys():
                if handle_name in param:
                    return True
        return False

    # Process commands, adding to appropriate dispatch tables
    def genCmd(self, cmd_info, name, alias):
        OutputGenerator.genCmd(self, cmd_info, name, alias)

        if 'android' in name.lower():
            return

        # Get first param type
        params = cmd_info.elem.findall('.//param')
        dispatch_type = params[0].find('type').text if params[0].find('type') is not None else ''
        handles_used = []
        cmd_params = []

        raw_handle = self.registry.tree.find("types/type/[name='" + dispatch_type + "'][@category='handle']")
        modified_handle = None
        return_type = cmd_info.elem.find('proto/type')

        if return_type is not None:
            if return_type.text == 'void':
                return_type = None
            elif self.paramIsHandle(return_type.text):
                for handle_name in self.handles.keys():
                    if handle_name in return_type.text:
                        handles_used.append(handle_name)

        is_create_command = any(
                                filter(
                                    lambda pat: pat in name, (
                                        'Create',
                                        'Allocate',
                                        'Enumerate',
                                        'RegisterDeviceEvent',
                                        'RegisterDisplayEvent',
                                        'AcquirePerformanceConfigurationINTEL'
                                    )
                                )
                            )
        is_create = is_create_command
        is_destroy_command = any([destroy_txt in name for destroy_txt in ['Destroy', 'Free', 'ReleasePerformanceConfigurationINTEL']])
        is_begin = 'Begin' in name
        is_end = 'End' in name

        lens = set()
        params = cmd_info.elem.findall('param')
        num_params = len(params)
        last = num_params - 1
        for index in range (0, num_params):
            param_type = params[index].find('type').text if params[index].find('type') is not None else ''
            param_name = params[index].find('name').text if params[index].find('name') is not None else ''
            param_cdecl = self.makeCParamDecl(params[index], 0)

            length_info = getLen(params[index])
            if length_info:
                lens.add(length_info)

            is_pointer = paramIsPointer(params[index])
            is_array = paramIsArray(params[index])
            array_sizes = paramGetArraySizes(params[index])
            is_const = True if 'const' in param_cdecl else False
            is_handle = self.paramIsHandle(param_type)

            if is_handle and param_type not in handles_used:
                handles_used.append(param_type)

            cmd_params.append(CommandParam(type = param_type,
                                           name = param_name,
                                           cdecl = param_cdecl,
                                           length_info = length_info,
                                           is_const = is_const,
                                           is_pointer = is_pointer,
                                           is_array = is_array,
                                           array_1st_size = array_sizes[0],
                                           array_2nd_size = array_sizes[1]))

            if is_handle and (is_pointer or is_destroy_command):
                modified_handle = param_type

            # Determine if this is actually a create or destroy command
            if index == last and ('vkGet' in name and is_handle and is_pointer and not is_const):
                is_create = True

        if is_create or is_destroy_command:
            if modified_handle in handles_used:
            # We don't want the modified handle listed as a used handle here
                handles_used.remove(modified_handle)
        else:
            # We don't want the modified_handle listed for anything but a create or destroy command
            modified_handle = ''

        self.commands[name] = BasicCommandData(
                name = name,
                protect = self.featureExtraProtect,
                return_type = return_type,
                raw_handle = raw_handle if 'CreateInfo' not in dispatch_type else None,
                handle_type = dispatch_type,
                is_create = is_create,
                is_destroy = is_destroy_command,
                is_begin = is_begin,
                is_end = is_end,
                modified_handle = modified_handle,
                handles_used = handles_used,
                params = cmd_params,
                cdecl = self.makeCDecls(cmd_info.elem)[0],
                alias = alias
            )

    def endFeature(self):
        # Finish processing in superclass
        OutputGenerator.endFeature(self)

    def GenLowestParentHandlesForHandleTree(self):
        for cur_handle in self.handle_tree:
            parent_list = cur_handle.parents
            for parent in parent_list:
                # Look for the handle_tree entry for this parent
                for tree_handle in self.handle_tree:
                    if tree_handle.name == parent:
                        # Check the handle_tree entry for parents
                        for check in tree_handle.parents:
                            if check in parent_list:
                                # Remove this element because it's a parent is in another handle list
                                cur_handle.parents.remove(check)

    def GenHighestChildrenHandlesForHandleTree(self):
        for cur_handle in self.handle_tree:
            child_list = cur_handle.children
            for child in child_list:
                # Look for the handle_tree entry for this child
                for tree_handle in self.handle_tree:
                    if tree_handle.name == child:
                        # Check the handle_tree entry for children
                        for check in tree_handle.children:
                            if check in child_list:
                                # Remove this element because it's a child is in another handle list
                                cur_handle.children.remove(check)

    # Generate the handle tree data so we can determine what handles
    # are needed for what other handles
    def GenerateHandleTreeData(self):
        for cur_handle in self.handles.values():
            parents = []
            children = []
            create_funcs = []
            destroy_funcs = []
            usage_funcs = []
            for command in self.commands.values():
                if command.modified_handle == cur_handle.name:
                    if command.is_create:
                        create_funcs.append(command)
                        if len(command.handle_type) > 0 and command.handle_type not in parents:
                            parents.append(command.handle_type)
                    if command.is_destroy:
                        destroy_funcs.append(command)
                elif command.handle_type == cur_handle.name:
                    usage_funcs.append(command)
                    if command.is_create and command.modified_handle not in children:
                        children.append(command.modified_handle)

            self.handle_tree.append(
                HandleTreeData(
                    name = cur_handle.name,
                    alias = cur_handle.alias,
                    parents = parents,
                    children = children,
                    create_funcs = create_funcs,
                    destroy_funcs = destroy_funcs,
                    usage_funcs = usage_funcs
                )
            )

        self.GenLowestParentHandlesForHandleTree()
        self.GenHighestChildrenHandlesForHandleTree()


    def OutputTestStart(self, major_ver, minor_ver, ext, use_dispatch_table):
        test_start = ''
        uses_surfaces = False
        create_surface_cmd = ''
        destroy_surface_cmd = ''
        req_additional_ext = []
        if 'VK_VERSION_' in ext.name:
            test_start += '// Test for Vulkan Core %d.%d\n' % (major_ver, minor_ver)
            disp_str = '_LoaderExports'
            if use_dispatch_table:
                disp_str = '_DispatchTable'
            test_start += 'TEST(BasicEntrypointTest, VulkanCore_%d_%d%s) {\n' % (major_ver, minor_ver, disp_str)
        else:
            test_start += '// Test for %s\n' % ext.name
            test_start += 'TEST(BasicEntrypointTest, %s) {\n' % ext.name[3:]

            if ext.type == 'instance':
                req_additional_ext.append(ext.define_name)

            for req_ext in ext.required_exts:
                bas_ext = self.extensions.get(req_ext)
                if (bas_ext is not None and bas_ext.type == 'instance' and
                    bas_ext.define_name not in req_additional_ext):
                    req_additional_ext.append(bas_ext.define_name)
                    if len(create_surface_cmd) == 0 and 'VK_KHR_surface' == req_ext:
                        create_surface_cmd = 'vkCreateHeadlessSurfaceEXT'
                    if len(destroy_surface_cmd) == 0 and 'VK_KHR_surface' == req_ext:
                        destroy_surface_cmd = 'vkDestroySurfaceKHR'

            for cmd_name, cmd_list in ext.command_data.items():
                for req in cmd_list:
                    bas_ext = self.extensions.get(req)
                    if (bas_ext is not None and bas_ext.type == 'instance' and
                        bas_ext.define_name not in req_additional_ext):
                        req_additional_ext.append(bas_ext.define_name)
                bas_cmd = self.commands.get(cmd_name)
                if bas_cmd is not None and 'VkSurfaceKHR' in bas_cmd.handles_used or 'VkSurfaceKHR' == bas_cmd.modified_handle:
                    if not uses_surfaces:
                        uses_surfaces = True
                        create_surface_cmd = 'vkCreateHeadlessSurfaceEXT'
                        destroy_surface_cmd = 'vkDestroySurfaceKHR'

                    if bas_cmd.is_create and bas_cmd.modified_handle == 'VkSurfaceKHR':
                        create_surface_cmd = bas_cmd.name

                    if bas_cmd.is_destroy and bas_cmd.modified_handle == 'VkSurfaceKHR':
                        destroy_surface_cmd = bas_cmd.name

            # If this uses vkCreateHeadlessSurfaceEXT make sure the
            # VK_KHR_surface extension is added to the required list.
            if (create_surface_cmd == 'vkCreateHeadlessSurfaceEXT' and
                'VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME' not in req_additional_ext):
                req_additional_ext.append('VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME')

            if destroy_surface_cmd == 'vkDestroySurfaceKHR':
                # If this is already the VK_KHR_surface extension, it contains the
                # vkDestroySurface call so we don't need to add it
                if ext.name == 'VK_KHR_surface':
                    destroy_surface_cmd = ''
                # Otherwise, make sure the VK_KHR_surface extension is added to the
                # required list.
                elif 'VK_KHR_SURFACE_EXTENSION_NAME' not in req_additional_ext:
                    req_additional_ext.append('VK_KHR_SURFACE_EXTENSION_NAME')

        test_start += '    FrameworkEnvironment env{};\n'
        test_start += '    uint32_t vulkan_version = VK_API_VERSION_%d_%d;\n' % (major_ver, minor_ver)
        test_start += '    env.add_icd(TestICDDetails(TEST_ENTRYPOINT_DRIVER, vulkan_version).set_type(ICDType::entrypoint_driver));\n'
        test_start += '\n'
        test_start += '    const char* entrypoint_test_layer_name = "VkLayer_LunarG_entrypoint_layer";\n'
        test_start += '    env.add_explicit_layer(\n'
        test_start += '        ManifestLayer{}.add_layer(\n'
        test_start += '            ManifestLayer::LayerDescription{}.set_name(entrypoint_test_layer_name).set_lib_path(TEST_ENTRYPOINT_LAYER)),\n'
        test_start += '        "regular_test_layer.json");\n'
        test_start += '\n'
        if use_dispatch_table:
            test_start += '    InstWrapper instance(env.vulkan_functions);\n'
            test_start += '    instance.create_info.set_api_version(vulkan_version);\n'
            test_start += '    instance.create_info.add_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);\n'
            for req_ext in req_additional_ext:
                test_start += '    instance.create_info.add_extension(%s);\n' % req_ext
            test_start += '    instance.create_info.add_layer(entrypoint_test_layer_name);\n'
            test_start += '    instance.CheckCreate();\n'
            test_start += '\n'
            test_start += '    DebugUtilsWrapper log{instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT};\n'
            test_start += '    CreateDebugUtilsMessenger(log);\n'
            test_start += '\n'
            test_start += '    VkLayerInstanceDispatchTable inst_disp_table;\n'
            test_start += '    layer_init_instance_dispatch_table(instance, &inst_disp_table, instance.functions->vkGetInstanceProcAddr);\n'
            test_start += '\n'
        else:
            test_start += '    std::vector<const char*> enabled_layers;\n'
            test_start += '    enabled_layers.push_back(entrypoint_test_layer_name);\n\n'
            test_start += '    std::vector<const char*> enabled_inst_exts;\n'
            test_start += '    enabled_inst_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);\n'
            for add_ext in req_additional_ext:
                test_start += '    enabled_inst_exts.push_back(%s);\n' % add_ext
            test_start += '\n'
            test_start += '    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};\n'
            test_start += '    app_info.apiVersion = vulkan_version;\n'
            test_start += '    VkInstanceCreateInfo inst_create_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};\n'
            test_start += '    inst_create_info.pApplicationInfo = &app_info;\n'
            test_start += '    inst_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());\n'
            test_start += '    inst_create_info.ppEnabledLayerNames = (enabled_layers.empty() ? nullptr : enabled_layers.data());\n'
            test_start += '    inst_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_inst_exts.size());\n'
            test_start += '    inst_create_info.ppEnabledExtensionNames = (enabled_inst_exts.empty() ? nullptr : enabled_inst_exts.data());\n'
            test_start += '\n'
            test_start += '    VkInstance instance;\n'
            test_start += '    vkCreateInstance(&inst_create_info, nullptr, &instance);\n'
            test_start += '\n'
            test_start += '    PFN_vkCreateDebugUtilsMessengerEXT debug_utils_create =\n'
            test_start += '        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));\n'
            test_start += '    PFN_vkDestroyDebugUtilsMessengerEXT debug_utils_destroy =\n'
            test_start += '        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));\n'
            test_start += '    DebugUtilsWrapper* log = new DebugUtilsWrapper{instance, debug_utils_create, debug_utils_destroy, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT};\n'
            test_start += '    ASSERT_TRUE(log != nullptr);\n'
            test_start += '    CreateDebugUtilsMessenger(*log);\n'
            test_start += '\n'

        # If there's a surface create command, trigger it now that we've created
        # the instance
        cmd = self.commands.get(create_surface_cmd)
        if cmd is not None:
            test_start += self.OutputTestEntrypoint(cmd, ext, use_dispatch_table)
        return test_start, destroy_surface_cmd

    def OutputTestEnd(self, major_ver, minor_ver, ext, destroy_surface_cmd, use_dispatch_table):
        test_end = ''

        # If there's a surface destroy command, trigger it before we exit the
        # test
        cmd = self.commands.get(destroy_surface_cmd)
        if cmd is not None:
            test_end += self.OutputTestEntrypoint(cmd, ext, use_dispatch_table)

        if not use_dispatch_table:
            test_end += '\n'
            test_end += '    delete log;\n'
            test_end += '    vkDestroyInstance(instance, nullptr);\n'

        test_end += '} '
        if 'VK_VERSION_' in ext.name:
            test_end += '// Test for Vulkan Core %d.%d\n' % (major_ver, minor_ver)
        else:
            test_end += '// Test for %s\n' % ext.name
        return test_end

    # This function finds any previously defined variable for the given parameter type.
    # If it's not found, it creates one.
    # It has 3 returns:
    #   name          The name of the variable to use for this parameter type
    #   define_var    The definition statement of a new variable if one is required
    #   length_info   The length information for a given parameter, this is a previously
    #                 defined parameter that is used for the count of this parameter.
    def FindOrAddParamVariable(self, param):
        found = False
        name = ''
        define_var = None
        length_info = None
        os_types = [
            'Display',
            'xcb_connection_t',
            'wl_display',
        ]
        # If it's a void or OS type, we really can't create it cleanly with
        # the default initializers, so use the "big_chunk_of_memory" array
        # and pass it with a type-cast
        if param.type == 'void' or param.type in os_types:
            already_defined = False
            for var in self.test_variables:
                if var.name == 'big_chunk_of_mem':
                    already_defined = True
                    break
            if not already_defined:
                self.test_variables.append(
                    TestVariableNames(
                        type = 'uint64_t',
                        name = 'big_chunk_of_mem',
                        is_array = True,
                        array_1st_size = 512,
                        array_2nd_size = 512
                    )
                )
                define_var = '    uint64_t big_chunk_of_mem[8][8];\n\n'
            name = 'big_chunk_of_mem'
        elif param.type == 'VkAllocationCallbacks':
            # The automatic tests don't try to use the allocation callback items
            return 'nullptr', None, None
        else:
            # First, look for the variable in the list of existing variables
            # making sure that it matches the type and at least the array
            # dimension sizes it needs
            for var in self.test_variables:
                if (var.type == param.type and param.is_array == var.is_array and
                    (param.array_1st_size <= var.array_1st_size and
                     param.array_2nd_size <= var.array_2nd_size)):
                    found = True
                    name = var.name
                    break
            if not found:
                name = 'var_'
                if 'vk' == param.type[0:1]:
                    name += param.type[2:].lower()
                else:
                    name += param.type.lower()
                if param.is_array:
                    if param.array_2nd_size > 0:
                        name += '_2d_%d_%d' % (param.array_1st_size, param.array_2nd_size)
                    else:
                        name += '_1d_%d' % param.array_1st_size
                self.test_variables.append(
                    TestVariableNames(
                        type = param.type,
                        name = name,
                        is_array = param.is_array,
                        array_1st_size = param.array_1st_size,
                        array_2nd_size = param.array_2nd_size
                    )
                )
                if param.is_array:
                    if param.array_2nd_size > 0:
                        define_var = '    %s %s[%d][%d]{};\n' % (param.type, name, param.array_1st_size, param.array_2nd_size)
                    else:
                        define_var = '    %s %s[%d]{};\n' % (param.type, name, param.array_1st_size)
                elif 'uint' in param.type:
                    define_var = '    %s %s = 1;\n' % (param.type, name)
                else:
                    init_value = ''
                    init_surf_value = ''
                    struct = self.struct_data.get(param.type)
                    if struct is not None:
                        for mem in struct.members:
                            if mem.type == 'VkStructureType':
                                init_value = mem.req_value
                            if mem.type == 'VkSurfaceKHR':
                                init_surf_value = "    %s.%s = var_vksurfacekhr;\n" % (name, mem.name)
                    define_var = '    %s %s{%s};\n' % (param.type, name, init_value)
                    define_var += init_surf_value
        if param.is_pointer and not param.is_array:
            new_name = ''
            # If the type is a pointer to a pointer, just do a reinterpret cast to make it happy
            if param.cdecl.count('*') > 1 or param.type in os_types:
                cdecl_short = re.sub(' +', ' ', param.cdecl.lstrip())
                cdecl_list = cdecl_short.split(' ')
                new_name += 'reinterpret_cast<'
                for i in range(0, len(cdecl_list) - 1):
                    if i > 0:
                        new_name += ' '
                    new_name += cdecl_list[i]
                new_name += '>(&%s)' % name
            elif param.type == 'void':
                new_name += "reinterpret_cast<void**>("
                new_name += "%s" % name
                new_name += ")"
            else:
                if param.is_pointer and param.length_info is not None:
                    length_info = '%s' % param.length_info
                new_name += "&%s" % name
            name = new_name
        return name, define_var, length_info

    def OutputTestEntrypoint(self, command, ext, use_dispatch_table):
        test_ep = ''

        if 'ProcAddr' in command.name:
            return ''

        if 'vkCreateDevice' == command.name:
            req_additional_ext = []
            if ext.type == 'device':
                req_additional_ext.append(ext.define_name)

            for req_ext in ext.required_exts:
                bas_ext = self.extensions.get(req_ext)
                if (bas_ext is not None and bas_ext.type == 'device' and
                    bas_ext.define_name not in req_additional_ext):
                    req_additional_ext.append(bas_ext.define_name)
            for cmd_list in ext.command_data:
                for req in cmd_list:
                    bas_ext = self.extensions.get(req)
                    if (bas_ext is not None and bas_ext.type == 'device' and
                        bas_ext.define_name not in req_additional_ext):
                        req_additional_ext.append(bas_ext.define_name)
            if use_dispatch_table:
                test_ep += '    DeviceWrapper dev{instance};\n'
                test_ep += '    dev.create_info.'
                for req_ext in req_additional_ext:
                    test_ep += 'add_extension(%s).' % req_ext
                test_ep += 'add_device_queue(DeviceQueueCreateInfo{}.add_priority(0.0f));\n'
                test_ep += '    dev.CheckCreate(var_vkphysicaldevice);\n'
                test_ep += '\n'
                test_ep += '    VkLayerDispatchTable device_disp_table;\n'
                test_ep += '    layer_init_device_dispatch_table(dev.dev, &device_disp_table, instance.functions->vkGetDeviceProcAddr);\n\n'
                self.test_variables.append(
                    TestVariableNames(
                        type = 'VkDevice',
                        name = 'dev.dev',
                        is_array = False,
                        array_1st_size = 1,
                        array_2nd_size = 0
                    )
                )
            else:
                test_ep += '    std::vector<const char*> enabled_dev_exts;\n'
                for add_ext in req_additional_ext:
                    test_ep += '    enabled_dev_exts.push_back(%s);\n' % add_ext
                test_ep += '\n'
                test_ep += '    VkDeviceQueueCreateInfo queue_create_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};\n'
                test_ep += '    queue_create_info.queueFamilyIndex = 0;\n'
                test_ep += '    queue_create_info.queueCount = 1;\n'
                test_ep += '    float priority = 0.f;\n'
                test_ep += '    queue_create_info.pQueuePriorities = &priority;\n'
                test_ep += '\n'
                test_ep += '    VkDeviceCreateInfo dev_create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};\n'
                test_ep += '    dev_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_dev_exts.size());\n'
                test_ep += '    dev_create_info.ppEnabledExtensionNames = (enabled_dev_exts.empty() ? nullptr : enabled_dev_exts.data());\n'
                test_ep += '    dev_create_info.queueCreateInfoCount = 1;\n'
                test_ep += '    dev_create_info.pQueueCreateInfos = &queue_create_info;\n'
                test_ep += '\n'
                test_ep += '    VkDevice device;\n'
                test_ep += '    vkCreateDevice(var_vkphysicaldevice, &dev_create_info, nullptr, &device);\n'
                test_ep += '    ASSERT_TRUE(log->find("Generated Layer vkCreateDevice"));\n'
                test_ep += '    ASSERT_TRUE(log->find("Generated Driver vkCreateDevice"));\n'
                test_ep += '    log->logger.clear();\n\n'
                self.test_variables.append(
                    TestVariableNames(
                        type = 'VkDevice',
                        name = 'device',
                        is_array = False,
                        array_1st_size = 1,
                        array_2nd_size = 0
                    )
                )
        elif 'vkDestroyDevice' == command.name:
            test_ep = ''
            if not use_dispatch_table:
                test_ep += '    vkDestroyDevice(device, nullptr);\n'
                test_ep += '    ASSERT_TRUE(log->find("Generated Layer vkDestroyDevice"));\n'
                test_ep += '    ASSERT_TRUE(log->find("Generated Driver vkDestroyDevice"));\n'
                test_ep += '    log->logger.clear();\n\n'
        else:
            param_names = []
            for param in command.params:
                name, define_var, length_info = self.FindOrAddParamVariable(param)

                # If there's a define for a new variable, add it right away
                if define_var is not None:
                    test_ep += define_var

                # If this parameter has a length, we need to adjust the length to match one of the
                # variables we have already defined using the correct name and dereferences.
                if length_info is not None:
                    length_var_name = length_info.split('-')[0]
                    length_var_name = length_var_name.split('.')[0]
                    for param_index in range (0, len(command.params)):
                        if length_var_name == command.params[param_index].name:
                            length_info = length_info.replace(length_var_name, param_names[param_index])
                            if '&' in length_info:
                                length_info = length_info.replace('&','').replace('->','.')
                            var_init_str = '    %s = 1;\n' % length_info
                            if var_init_str not in test_ep:
                                test_ep += var_init_str

                param_names.append(name)

            if use_dispatch_table:
                # Call the command with the correct dispatch table
                if (command.handle_type == 'VkInstance' or command.handle_type == 'VkPhysicalDevice' or
                    command.handle_type == 'VkSurfaceKHR'):
                    test_ep += '    inst_disp_table.%s(' % command.name[2:]
                else:
                    test_ep += '    device_disp_table.%s(' % command.name[2:]
            else:
                test_ep += '    %s(' % command.name

            test_ep += ', '.join(param_names)
            test_ep += ');\n'

            log_call = 'log.'
            if not use_dispatch_table:
                log_call = 'log->'

            if command.alias is not None:
                # If it has an alias, the layer and drivers may use the same function for
                # each, so check both and one should be true
                test_ep += '    ASSERT_TRUE(%sfind("Generated Layer %s") ||\n' % (log_call, command.name)
                test_ep += '                %sfind("Generated Layer %s"));\n' % (log_call, command.alias)
                test_ep += '    ASSERT_TRUE(%sfind("Generated Driver %s") ||\n' % (log_call, command.name)
                test_ep += '                %sfind("Generated Driver %s"));\n' % (log_call, command.alias)
            else:
                test_ep += '    ASSERT_TRUE(%sfind("Generated Layer %s"));\n' % (log_call, command.name)
                test_ep += '    ASSERT_TRUE(%sfind("Generated Driver %s"));\n' % (log_call, command.name)
            test_ep += '    %slogger.clear();\n\n' % log_call

        return test_ep

    def CallUsageFuncs(self, usage_funcs, major_ver, minor_ver, ext, use_dispatch_table):
        call_funcs = ''
        for usage in usage_funcs:
            if usage.is_create or usage.is_destroy:
                continue

            cmd_list = ext.command_data.get(usage.name)
            if cmd_list is not None:
                ext_major_ver = 1
                ext_minor_ver = 0
                for requires in cmd_list:
                    if 'VK_VERSION_' in requires:
                        version_numbers = list(map(int, re.findall(r'\d+', requires)))
                        ext_major_ver = version_numbers[0]
                        ext_minor_ver = version_numbers[1]
                        break
                if (major_ver == ext_major_ver and minor_ver >= ext_minor_ver):
                    call_funcs += self.OutputTestEntrypoint(usage, ext, use_dispatch_table)
        return call_funcs

    def PickCreateDestroyFunc(self, cur_handle_name, create_funcs, destroy_funcs, major_ver, minor_ver, ext_name):
        create_list = []
        destroy_list = []

        # First try an exact match of version and extension
        try_minor_version = minor_ver
        while try_minor_version >= 0:
            # Go through list and only add items that are supported in the API version with the
            # above extension we are interested in
            for ext in self.extensions.values():
                ext_major_ver = 1
                ext_minor_ver = 0
                check_ext = False
                if 'VK_VERSION_' in ext.name:
                    version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
                    ext_major_ver = version_numbers[0]
                    ext_minor_ver = version_numbers[1]
                else:
                    check_ext = True

                for create_func in create_funcs:
                    cmd = ext.command_data.get(create_func.name)
                    if (cmd is not None and major_ver == ext_major_ver and try_minor_version == ext_minor_ver and
                        (not check_ext or ext_name == ext.name)):
                        create_list.append(create_func)
                    if len(create_list) > 0:
                        break
                for destroy_func in destroy_funcs:
                    cmd = ext.command_data.get(destroy_func.name)
                    if (cmd is not None and major_ver == ext_major_ver and try_minor_version == ext_minor_ver and
                        (not check_ext or ext_name == ext.name)):
                        destroy_list.append(destroy_func)
                    if len(destroy_list) > 0:
                        break
            if len(create_list) > 0 and len(destroy_list) > 0:
                break
            try_minor_version = try_minor_version - 1

        chosen_create = None
        chosen_destroy = None
        if len(create_list) > 0:
            chosen_create = create_list[0]
        if len(destroy_list) > 0:
            chosen_destroy = destroy_list[0]

        return chosen_create, chosen_destroy


    def GenHandleTest(self, cur_handle_name, handles_to_test, major_ver, minor_ver, ext, use_dispatch_table):
        test = ''
        for cur_handle in self.handle_tree:
            if cur_handle_name == cur_handle.name:
                create = None
                destroy = None
                if cur_handle_name != 'VkInstance':
                    create, destroy = self.PickCreateDestroyFunc(
                                            cur_handle_name,
                                            cur_handle.create_funcs,
                                            cur_handle.destroy_funcs,
                                            major_ver,
                                            minor_ver,
                                            ext.name)
                    # If there's nothing to do, don't do it.
                    if create is None and destroy is None and len(cur_handle.usage_funcs) == 0:
                        continue

                if cur_handle_name != 'VkInstance' or 'VK_VERSION_' not in ext.name:
                    if create is not None:
                        test += self.OutputTestEntrypoint(create, ext, use_dispatch_table)

                    test += self.CallUsageFuncs(cur_handle.usage_funcs, major_ver, minor_ver, ext, use_dispatch_table)

                for child in cur_handle.children:
                    if child in handles_to_test:
                        test += self.GenHandleTest(child, handles_to_test, major_ver, minor_ver, ext, use_dispatch_table)

                if destroy is not None:
                    test += self.OutputTestEntrypoint(destroy, ext, use_dispatch_table)
        return test

    # Generate the list of required handles to test.  We need to have at least a creation path
    # from VkPhysicalDevice down to just the handles we need
    def GenerateTestHandleList(self, ext):
        handle_list = []

        # First generate the list of handles required
        for ext_cmd in ext.command_data.keys():
            cmd = self.commands.get(ext_cmd)
            if cmd is not None:
                if cmd.handle_type is not None and cmd.handle_type not in handle_list:
                    handle_list.append(cmd.handle_type)
                for used_handle in cmd.handles_used:
                    if used_handle not in handle_list:
                         handle_list.append(used_handle)

        # Second, make sure all the ancestors of the required handles are included up to at least
        # VkPhysicalDevice
        for handle in handle_list:
            for tree_handle in self.handle_tree:
                if handle == tree_handle.name:
                    for parent in tree_handle.parents:
                        if parent not in handle_list:
                            handle_list.append(parent)
                    break

        return handle_list

    # Generate the test
    def GenerateCoreTest(self, ext, use_dispatch_table):
        test = ''
        self.test_variables = []
        self.test_variables.append(
            TestVariableNames(
                type = 'VkInstance',
                name = 'instance',
                is_array = False,
                array_1st_size = 1,
                array_2nd_size = 0
            )
        )

        version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
        major_ver = version_numbers[0]
        minor_ver = version_numbers[1]

        required_handle_list = self.GenerateTestHandleList(ext)

        test_str, destroy_surface_cmd = self.OutputTestStart(major_ver, minor_ver, ext, use_dispatch_table)
        test += test_str
        test += self.GenHandleTest('VkInstance', required_handle_list, major_ver, minor_ver, ext, use_dispatch_table)
        test += self.OutputTestEnd(major_ver, minor_ver, ext, destroy_surface_cmd, use_dispatch_table)
        test += '\n'
        return test

    # Generate the test
    def GenerateExtensionTest(self, ext, use_dispatch_table):
        test = ''
        self.test_variables = []
        self.test_variables.append(
            TestVariableNames(
                type = 'VkInstance',
                name = 'instance',
                is_array = False,
                array_1st_size = 1,
                array_2nd_size = 0
            )
        )

        required_handle_list = self.GenerateTestHandleList(ext)

        if (ext.protect is not None):
            test += '#ifdef %s\n' % ext.protect

        test_str, destroy_surface_cmd = self.OutputTestStart(self.max_major, self.max_minor, ext, use_dispatch_table)
        test += test_str
        test += self.GenHandleTest('VkInstance', required_handle_list, self.max_major, self.max_minor, ext, use_dispatch_table)
        test += self.OutputTestEnd(self.max_major, self.max_minor, ext, destroy_surface_cmd, use_dispatch_table)

        if (ext.protect is not None):
            test += '#endif // %s\n' % ext.protect
        test += '\n'

        return test

    def GenerateLayerHeader(self):
        ret = '''struct EntrypointTestLayer {
    fs::path manifest_file_path;
    const uint32_t manifest_version = VK_MAKE_API_VERSION(0, 1, 1, 2);
    const uint32_t api_version = VK_API_VERSION_%d_%d;

    PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr = VK_NULL_HANDLE;
    PFN_vkGetDeviceProcAddr next_vkGetDeviceProcAddr = VK_NULL_HANDLE;

    const uint32_t max_icd_interface_version = 6;
    VkInstance instance_handle;
    VkLayerInstanceDispatchTable instance_dispatch_table{};
    uint8_t enabled_instance_major;
    uint8_t enabled_instance_minor;
    std::vector<std::string> enabled_instance_extensions{};

    struct DebugUtilsInfo {
        VkDebugUtilsMessageSeverityFlagsEXT     severities;
        VkDebugUtilsMessageTypeFlagsEXT         types;
        PFN_vkDebugUtilsMessengerCallbackEXT    callback = nullptr;
        void*                                   user_data = nullptr;
    };
    DebugUtilsInfo debug_util_info;
    struct Device {
        VkDevice device_handle;
        VkLayerDispatchTable dispatch_table;
        std::vector<std::string> enabled_extensions;
    };
    std::vector<Device> created_devices;
};

using GetTestLayerFunc = EntrypointTestLayer* (*)();
#define GET_TEST_LAYER_FUNC_STR "get_test_layer_func"

using GetNewTestLayerFunc = EntrypointTestLayer* (*)();
#define RESET_LAYER_FUNC_STR "reset_layer_func"

''' % (self.max_major, self.max_minor)
        return ret

    def OutputCommonInstanceDeviceLayerFuncs(self):
        ret = '''EntrypointTestLayer layer;
extern "C" {
FRAMEWORK_EXPORT EntrypointTestLayer* get_test_layer_func() { return &layer; }
FRAMEWORK_EXPORT EntrypointTestLayer* reset_layer_func() {
    layer.~EntrypointTestLayer();
    return new (&layer) EntrypointTestLayer();
}
} // extern "C"

#ifndef TEST_LAYER_NAME
#define TEST_LAYER_NAME "VkLayer_LunarG_entrypoint_layer"
#endif

VkLayerInstanceCreateInfo* get_chain_info(const VkInstanceCreateInfo* pCreateInfo, VkLayerFunction func) {
    VkLayerInstanceCreateInfo* chain_info = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerInstanceCreateInfo*)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

VkLayerDeviceCreateInfo* get_chain_info(const VkDeviceCreateInfo* pCreateInfo, VkLayerFunction func) {
    VkLayerDeviceCreateInfo* chain_info = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerDeviceCreateInfo*)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

void log_layer_message(const std::string& message) {
    static uint8_t cur_message_index = 0;
    static std::string messages[8];
    if (layer.debug_util_info.callback != nullptr) {
        VkDebugUtilsMessengerCallbackDataEXT callback_data{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
        messages[cur_message_index] = message.c_str();
        callback_data.pMessage = messages[cur_message_index].c_str();
        if (++cur_message_index >= 8) { cur_message_index = 0; }
        layer.debug_util_info.callback(
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
               VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
               &callback_data,
               layer.debug_util_info.user_data);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL layer_EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL layer_EnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount,
                                                                         VkExtensionProperties* pProperties) {
    if (pLayerName && string_eq(pLayerName, TEST_LAYER_NAME)) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }
    return layer.instance_dispatch_table.EnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL layer_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,
                                                                   VkLayerProperties* pProperties) {
    log_layer_message("Generated Layer vkEnumerateDeviceLayerProperties");
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL layer_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName,
                                                                       uint32_t* pPropertyCount,
                                                                       VkExtensionProperties* pProperties) {
    log_layer_message("Generated Layer vkEnumerateDeviceExtensionProperties");
    if (pLayerName && string_eq(pLayerName, TEST_LAYER_NAME)) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }
    return layer.instance_dispatch_table.EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount,
                                                                            pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL layer_EnumerateInstanceVersion(uint32_t* pApiVersion) {
    log_layer_message("Generated Layer vkEnumerateInstanceVersion");
    if (pApiVersion != nullptr) {
        *pApiVersion = VK_API_VERSION_%d_%d;
    }
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL layer_CreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                   const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    VkLayerInstanceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    layer.next_vkGetInstanceProcAddr = fpGetInstanceProcAddr;

    // Advance the link info for the next element of the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // Continue call down the chain
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }
    layer.instance_handle = *pInstance;

    layer.enabled_instance_major = 1;
    layer.enabled_instance_minor = 0;
    if (pCreateInfo->pApplicationInfo != NULL && pCreateInfo->pApplicationInfo->apiVersion != 0) {
        layer.enabled_instance_major = static_cast<uint8_t>(VK_API_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion));
        layer.enabled_instance_minor = static_cast<uint8_t>(VK_API_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion));
    }

    for (uint32_t ext = 0; ext < pCreateInfo->enabledExtensionCount; ++ext) {
        layer.enabled_instance_extensions.push_back(pCreateInfo->ppEnabledExtensionNames[ext]);
    }

    // Init layer's dispatch table using GetInstanceProcAddr of next layer in the chain.
    layer_init_instance_dispatch_table(layer.instance_handle, &layer.instance_dispatch_table, fpGetInstanceProcAddr);

    return result;
}

VKAPI_ATTR void VKAPI_CALL layer_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    layer.instance_dispatch_table.DestroyInstance(instance, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL layer_CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                                  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                                  const VkAllocationCallbacks* pAllocator,
                                                                  VkDebugUtilsMessengerEXT* pMessenger) {
    layer.debug_util_info.severities = pCreateInfo->messageSeverity;
    layer.debug_util_info.types = pCreateInfo->messageType;
    layer.debug_util_info.callback = pCreateInfo->pfnUserCallback;
    layer.debug_util_info.user_data = pCreateInfo->pUserData;
    return layer.instance_dispatch_table.CreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR VkResult VKAPI_CALL layer_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
                                                 const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    VkLayerDeviceCreateInfo* chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    log_layer_message("Generated Layer vkCreateDevice");

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(layer.instance_handle, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    layer.next_vkGetDeviceProcAddr = fpGetDeviceProcAddr;

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }
    EntrypointTestLayer::Device device{*pDevice};

    for (uint32_t ext = 0; ext < pCreateInfo->enabledExtensionCount; ++ext) {
        device.enabled_extensions.push_back(pCreateInfo->ppEnabledExtensionNames[ext]);
    }

    // initialize layer's dispatch table
    layer_init_device_dispatch_table(device.device_handle, &device.dispatch_table, fpGetDeviceProcAddr);

    // Need to add the created devices to the list so it can be freed
    layer.created_devices.push_back(device);

    return result;
}

VKAPI_ATTR void VKAPI_CALL layer_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
    log_layer_message("Generated Layer vkDestroyDevice");
    for (auto& created_device : layer.created_devices) {
        if (created_device.device_handle == device) {
            created_device.dispatch_table.DestroyDevice(device, pAllocator);
            break;
        }
    }
}
''' % (self.max_major, self.max_minor)
        return ret

    def OutputLayerGIPA(self):
        layer_gipa = ''
        layer_gipa += 'bool IsInstanceExtensionEnabled(const char* extension_name) {\n'
        layer_gipa += '    return layer.enabled_instance_extensions.end() !=\n'
        layer_gipa += '           std::find_if(layer.enabled_instance_extensions.begin(),\n'
        layer_gipa += '                        layer.enabled_instance_extensions.end(),\n'
        layer_gipa += '                        [extension_name](Extension const& ext) { return ext.extensionName == extension_name; });\n'
        layer_gipa += '}\n\n'
        layer_gipa += '// Prototypes:\n'
        layer_gipa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL layer_GetDeviceProcAddr(VkDevice device, const char* pName);\n'
        layer_gipa += '\n\n'
        layer_gipa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL layer_GetInstanceProcAddr(VkInstance instance, const char* pName) {\n'

        printed_commands = set()
        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            layer_gipa += '\n    // ----- %s\n' % ext.name

            if (ext.protect is not None):
                layer_gipa += '#ifdef %s\n' % ext.protect

            major = 1
            minor = 0
            add_indent = ''
            if 'VK_VERSION_' in ext.name:
                version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
                major = version_numbers[0]
                minor = version_numbers[1]

                if (minor > 0):
                    layer_gipa += '    if (layer.enabled_instance_minor >= %d) {\n' % minor
                else:
                    layer_gipa += '    {\n'
                add_indent = '    '
            elif ext.type == 'instance':
                layer_gipa += '    if (IsInstanceExtensionEnabled("%s")) {\n' % ext.name
                add_indent = '    '

            for cmd_name in ext.command_data.keys():
                if cmd_name in printed_commands:
                    continue
                layer_gipa += '    %sif (string_eq(pName, "%s")) return to_vkVoidFunction(layer_%s);\n' % (add_indent, cmd_name, cmd_name[2:])
                printed_commands.add(cmd_name)

            if 'VK_VERSION_' in ext.name:
                layer_gipa += '    } '
                layer_gipa += '// End Core %d.%d\n' % (major, minor)
            elif ext.type == 'instance':
                layer_gipa += '    } // %s\n' % ext.name

            if (ext.protect is not None):
                layer_gipa += '#endif // %s\n' % ext.protect

        layer_gipa += '\n'
        layer_gipa += '    return layer.instance_dispatch_table.GetInstanceProcAddr(instance, pName);\n'
        layer_gipa += '}\n'
        layer_gipa += '\n'
        return layer_gipa

    def OutputLayerGDPA(self):
        layer_gdpa = ''
        layer_gdpa += 'bool IsDeviceExtensionEnabled(const char* extension_name) {\n'
        layer_gdpa += '    return layer.created_devices[0].enabled_extensions.end() !=\n'
        layer_gdpa += '           std::find_if(layer.created_devices[0].enabled_extensions.begin(),\n'
        layer_gdpa += '                        layer.created_devices[0].enabled_extensions.end(),\n'
        layer_gdpa += '                        [extension_name](Extension const& ext) { return ext.extensionName == extension_name; });\n'
        layer_gdpa += '}\n\n'
        layer_gdpa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL layer_GetDeviceProcAddr(VkDevice device, const char* pName) {\n'

        printed_commands = set()
        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            layer_gdpa += '\n    // ----- %s\n' % ext.name

            if (ext.protect is not None):
                layer_gdpa += '#ifdef %s\n' % ext.protect

            major = 1
            minor = 0
            add_indent = ''
            if 'VK_VERSION_' in ext.name:
                version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
                major = version_numbers[0]
                minor = version_numbers[1]

                if (minor > 0):
                    layer_gdpa += '    if (layer.enabled_instance_minor >= %d) {\n' % minor
                else:
                    layer_gdpa += '    {\n'
                add_indent = '    '
            elif ext.type == 'device':
                layer_gdpa += '    if (IsDeviceExtensionEnabled("%s")) {\n' % ext.name
                add_indent = '    '

            for cmd_name in ext.command_data.keys():
                if cmd_name in printed_commands:
                    continue
                layer_gdpa += '    %sif (string_eq(pName, "%s")) return to_vkVoidFunction(layer_%s);\n' % (add_indent, cmd_name, cmd_name[2:])
                printed_commands.add(cmd_name)

            if 'VK_VERSION_' in ext.name:
                layer_gdpa += '    } '
                layer_gdpa += '// End Core %d.%d\n' % (major, minor)
            elif ext.type == 'device':
                layer_gdpa += '    } // %s\n' % ext.name

            if (ext.protect is not None):
                layer_gdpa += '#endif // %s\n' % ext.protect

        layer_gdpa += '\n'
        layer_gdpa += '    return layer.created_devices[0].dispatch_table.GetDeviceProcAddr(device, pName);\n'
        layer_gdpa += '}\n'
        layer_gdpa += '\n'
        return layer_gdpa

    def OutputLayerWrapup(self):
        layer_wrapup = ''
        layer_wrapup += '\n'
        layer_wrapup += '// Exported functions\n'
        layer_wrapup += 'extern "C" {\n'
        layer_wrapup += '\n'
        layer_wrapup += self.OutputLayerGIPA()
        layer_wrapup += self.OutputLayerGDPA()
        layer_wrapup += '\n'
        layer_wrapup += '#if (defined(__GNUC__) && (__GNUC__ >= 4)) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))\n'
        layer_wrapup += '#define EXPORT_NEGOTIATE_FUNCTION __attribute__((visibility("default")))\n'
        layer_wrapup += '#else\n'
        layer_wrapup += '#define EXPORT_NEGOTIATE_FUNCTION\n'
        layer_wrapup += '#endif\n'
        layer_wrapup += '\n'
        layer_wrapup += 'EXPORT_NEGOTIATE_FUNCTION VKAPI_ATTR VkResult VKAPI_CALL\n'
        layer_wrapup += 'vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {\n'
        layer_wrapup += '    if (pVersionStruct) {\n'
        layer_wrapup += '        pVersionStruct->loaderLayerInterfaceVersion = layer.max_icd_interface_version;\n'
        layer_wrapup += '        pVersionStruct->pfnGetInstanceProcAddr = layer_GetInstanceProcAddr;\n'
        layer_wrapup += '        pVersionStruct->pfnGetDeviceProcAddr = layer_GetDeviceProcAddr;\n'
        layer_wrapup += '        pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;\n'
        layer_wrapup += '\n'
        layer_wrapup += '        return VK_SUCCESS;\n'
        layer_wrapup += '    }\n'
        layer_wrapup += '    return VK_ERROR_INITIALIZATION_FAILED;\n'
        layer_wrapup += '}\n\n'
        layer_wrapup += '}  // extern "C"\n'
        return layer_wrapup

    def PrintLayerCommand(self, command_name):
        cmd = self.commands.get(command_name)
        if cmd is None:
            return ''
        cmd_str = ''
        cmd_str += cmd.cdecl.replace(';', ' {\n').replace('VKAPI_CALL vk', 'VKAPI_CALL layer_')
        cmd_str += '    log_layer_message("Generated Layer %s");\n' % cmd.name
        if cmd.return_type is not None:
            cmd_str += '    return '
        else:
            cmd_str += '   '
        if (cmd.handle_type in ['VkInstance', 'VkPhysicalDevice']):
            cmd_str += 'layer.instance_dispatch_table.%s(' % cmd.name[2:]
        else:
            cmd_str += 'layer.created_devices[0].dispatch_table.%s(' % cmd.name[2:]
        param_list = []
        for param in cmd.params:
            param_list.append(param.name)
        cmd_str += ', '.join(param_list)
        cmd_str += ');\n'
        cmd_str += '}\n'
        return cmd_str

    def GenerateLayerSource(self):
        layer_src = ''
        layer_src += self.OutputCommonInstanceDeviceLayerFuncs()

        # Printout funcs
        printed_commands = set()
        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            layer_src += '// ----- %s\n' % ext.name

            if (ext.protect is not None):
                layer_src += '#ifdef %s\n' % ext.protect

            for cmd_name in ext.command_data.keys():
                if cmd_name in MANUAL_LAYER_DRIVER_ENTRYPOINTS or cmd_name in printed_commands:
                    continue

                layer_src += self.PrintLayerCommand(cmd_name)
                layer_src += '\n'
                printed_commands.add(cmd_name)

            if (ext.protect is not None):
                layer_src += '#endif // %s\n' % ext.protect

            layer_src += '\n'

        layer_src += self.OutputLayerWrapup()
        return layer_src

    def GenerateDriverHeader(self):
        max_api_version = 'VK_API_VERSION_%d_%d' % (self.max_major, self.max_minor)
        driver_header = ''
        driver_header += 'struct EntrypointTestDriver {\n'
        driver_header += '    fs::path manifest_file_path;\n'
        driver_header += '\n'
        driver_header += '    const uint32_t icd_api_version = %s;\n' % max_api_version
        driver_header += '    const uint32_t max_icd_interface_version = 6;\n'
        driver_header += '    uint8_t enabled_instance_major;\n'
        driver_header += '    uint8_t enabled_instance_minor;\n'
        driver_header += '\n'
        driver_header += '    DispatchableHandle<VkInstance> instance_handle;\n'
        driver_header += '    bool instance_extensions_initialized = false;\n'
        driver_header += '    std::vector<std::string> instance_extensions{};\n'
        driver_header += '    bool device_extensions_initialized = false;\n'
        driver_header += '    std::vector<std::string> device_extensions{};\n'
        driver_header += '    DispatchableHandle<VkPhysicalDevice> physical_device_handle;\n'
        driver_header += '    std::vector<std::string> enabled_instance_extensions{};\n'
        driver_header += '\n'
        driver_header += '    struct DebugUtilsInfo {\n'
        driver_header += '        VkDebugUtilsMessageSeverityFlagsEXT     severities;\n'
        driver_header += '        VkDebugUtilsMessageTypeFlagsEXT         types;\n'
        driver_header += '        PFN_vkDebugUtilsMessengerCallbackEXT    callback = nullptr;\n'
        driver_header += '        void*                                   user_data = nullptr;\n'
        driver_header += '    };\n'
        driver_header += '    DebugUtilsInfo debug_util_info;'
        driver_header += '\n'
        driver_header += '    std::vector<DispatchableHandle<VkDevice>*> dev_handles;\n'
        driver_header += '    std::vector<std::string> enabled_device_extensions;\n'
        driver_header += '\n'
        for handle in self.handles.values():
            if handle.name in ['VkInstance', 'VkPhysicalDevice', 'VkDevice']:
                continue
            if handle.is_dispatchable:
                driver_header += '    std::vector<DispatchableHandle<%s>*> %s_handles;\n' % (handle.name, handle.name[2:].lower())
        driver_header += '};\n'
        driver_header += '\n'
        driver_header += 'using GetEPDriverFunc = EntrypointTestDriver* (*)();\n'
        driver_header += '#define GET_EP_DRIVER_STR "get_ep_driver_func"\n'
        driver_header += '\n'
        driver_header += 'using GetNewEPDriverFunc = EntrypointTestDriver* (*)();\n'
        driver_header += '#define RESET_EP_DRIVER_STR "reset_ep_driver_func"\n'
        return driver_header

    def OutputDriverAliasPrototypes(self):
        alias_protos = ''
        alias_protos += '// Alias prototypes\n'
        alias_protos += '//---------------------\n'
        for cmd in self.commands.values():
            if cmd.alias is not None:
                alias_cmd = cmd.cdecl.replace(cmd.name, cmd.alias).replace('VKAPI_CALL vk', 'VKAPI_CALL driver_')
                alias_cmd = re.sub('\n', ' ', alias_cmd)
                alias_cmd = re.sub(' +', ' ', alias_cmd)
                alias_protos += alias_cmd.replace('( ', '(').replace(' )', ')')
                alias_protos += '\n'
        alias_protos += '\n'
        return alias_protos

    def OutputCommonInstanceDeviceDriverFuncs(self):
        max_api_version = 'VK_API_VERSION_%d_%d' % (self.max_major, self.max_minor)
        common_src = ''
        common_src += '\n//Temporary device name\n'
        common_src += 'const char driver_phys_device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = "TestDriverFakePhysicalDevice";\n'
        common_src += '\n'
        common_src += '// Instance extensions supported\n'
        common_src += 'const char inst_ext_arr[][VK_MAX_EXTENSION_NAME_SIZE] = {\n'
        for ext in self.extensions.values():
            if ext.type == 'instance':
                if (ext.protect is not None):
                    common_src += '#ifdef %s\n' % ext.protect

                common_src += '    "%s",\n' % ext.name

                if (ext.protect is not None):
                    common_src += '#endif // %s\n' % ext.protect
        common_src += '};\n'
        common_src += '\n// Device extensions supported\n'
        common_src += 'const char dev_ext_arr[][VK_MAX_EXTENSION_NAME_SIZE] = {\n'
        for ext in self.extensions.values():
            if ext.type == 'device':
                if (ext.protect is not None):
                    common_src += '#ifdef %s\n' % ext.protect

                common_src += '    "%s",\n' % ext.name

                if (ext.protect is not None):
                    common_src += '#endif // %s\n' % ext.protect
        common_src += '};\n'
        common_src += '\n'
        common_src += 'void driver_initialize_instance_exts() {\n'
        common_src += '    if (driver.instance_extensions_initialized) { return; }\n'
        common_src += '    for(const auto& str : inst_ext_arr){\n'
        common_src += '        driver.instance_extensions.push_back(str);\n'
        common_src += '    }\n'
        common_src += '    driver.instance_extensions_initialized = true;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'void driver_initialize_device_exts() {\n'
        common_src += '    if (driver.device_extensions_initialized) { return; }\n'
        common_src += '    for(const auto& str : dev_ext_arr){\n'
        common_src += '        driver.device_extensions.push_back(str);\n'
        common_src += '    }\n'
        common_src += '    driver.device_extensions_initialized = true;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'void log_driver_message(const std::string& message) {\n'
        common_src += '    static uint8_t cur_message_index = 0;\n'
        common_src += '    static std::string messages[8];\n'
        common_src += '    if (driver.debug_util_info.callback != nullptr) {\n'
        common_src += '        VkDebugUtilsMessengerCallbackDataEXT callback_data{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};\n'
        common_src += '        messages[cur_message_index] = message.c_str();\n'
        common_src += '        callback_data.pMessage = messages[cur_message_index].c_str();\n'
        common_src += '        if (++cur_message_index >= 8) { cur_message_index = 0; }\n'
        common_src += '        driver.debug_util_info.callback(\n'
        common_src += '               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,\n'
        common_src += '               VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,\n'
        common_src += '               &callback_data,\n'
        common_src += '               driver.debug_util_info.user_data);\n'
        common_src += '    }\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += '//// Instance Functions ////\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount,\n'
        common_src += '                                                                           VkExtensionProperties* pProperties) {\n'
        common_src += '    driver_initialize_instance_exts();\n'
        common_src += '\n'
        common_src += '    VkResult res = VK_SUCCESS;\n'
        common_src += '    if (pLayerName == nullptr) {\n'
        common_src += '        if (pProperties != nullptr) {\n'
        common_src += '            uint32_t count = static_cast<uint32_t>(driver.instance_extensions.size());\n'
        common_src += '            if (*pPropertyCount < count) {\n'
        common_src += '                count = *pPropertyCount;\n'
        common_src += '                res = VK_INCOMPLETE;\n'
        common_src += '            }\n'
        common_src += '            for (uint32_t ext = 0; ext < count; ++ext) {\n'
        common_src += '                pProperties[ext].specVersion = 1;\n'
        common_src += '#if defined(_WIN32)\n'
        common_src += '            strncpy_s(pProperties[ext].extensionName, VK_MAX_EXTENSION_NAME_SIZE, driver.instance_extensions[ext].c_str(), driver.instance_extensions[ext].size() + 1);\n'
        common_src += '#else\n'
        common_src += '            strncpy(pProperties[ext].extensionName, driver.instance_extensions[ext].c_str(), VK_MAX_EXTENSION_NAME_SIZE);\n'
        common_src += '#endif\n'
        common_src += '            }\n'
        common_src += '        } else {\n'
        common_src += '            *pPropertyCount = static_cast<uint32_t>(driver.instance_extensions.size());\n'
        common_src += '        }\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    return res;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {\n'
        common_src += '    *pPropertyCount = 0;\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumerateInstanceVersion(uint32_t* pApiVersion) {\n'
        common_src += '    if (pApiVersion != nullptr) {\n'
        common_src += '        *pApiVersion = %s;\n' % max_api_version
        common_src += '    }\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_CreateInstance(const VkInstanceCreateInfo* pCreateInfo,\n'
        common_src += '                                                     const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {\n'
        common_src += '    if (pCreateInfo == nullptr || pCreateInfo->pApplicationInfo == nullptr) {\n'
        common_src += '        return VK_ERROR_OUT_OF_HOST_MEMORY;\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    if (pCreateInfo->pApplicationInfo->apiVersion > %s) {\n' % max_api_version
        common_src += '        return VK_ERROR_INCOMPATIBLE_DRIVER;\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    driver_initialize_instance_exts();\n'
        common_src += '\n'
        common_src += '    driver.enabled_instance_major = 1;\n'
        common_src += '    driver.enabled_instance_minor = 0;\n'
        common_src += '    if (pCreateInfo->pApplicationInfo != NULL && pCreateInfo->pApplicationInfo->apiVersion != 0) {\n'
        common_src += '        driver.enabled_instance_major = static_cast<uint8_t>(VK_API_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion));\n'
        common_src += '        driver.enabled_instance_minor = static_cast<uint8_t>(VK_API_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion));\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    for (uint32_t ext = 0; ext < pCreateInfo->enabledExtensionCount; ++ext) {\n'
        common_src += '        driver.enabled_instance_extensions.push_back(pCreateInfo->ppEnabledExtensionNames[ext]);\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    // VK_SUCCESS\n'
        common_src += '    *pInstance = driver.instance_handle.handle;\n'
        common_src += '\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR void VKAPI_CALL driver_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount,\n'
        common_src += '                                                               VkPhysicalDevice* pPhysicalDevices) {\n'
        common_src += '    log_driver_message("Generated Driver vkEnumeratePhysicalDevices");\n'
        common_src += '    if (pPhysicalDevices != nullptr) {\n'
        common_src += '        pPhysicalDevices[0] = driver.physical_device_handle.handle;\n'
        common_src += '    }\n'
        common_src += '    *pPhysicalDeviceCount = 1;\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumeratePhysicalDeviceGroups(\n'
        common_src += '    VkInstance instance, uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {\n'
        common_src += '    log_driver_message("Generated Driver vkEnumeratePhysicalDeviceGroups");\n'
        common_src += '\n'
        common_src += '    if (pPhysicalDeviceGroupProperties != nullptr) {\n'
        common_src += '        pPhysicalDeviceGroupProperties[0].subsetAllocation = false;\n'
        common_src += '        pPhysicalDeviceGroupProperties[0].physicalDeviceCount = 1;\n'
        common_src += '        pPhysicalDeviceGroupProperties[0].physicalDevices[0] = driver.physical_device_handle.handle;\n'
        common_src += '    }\n'
        common_src += '    *pPhysicalDeviceGroupCount = 1;\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR void VKAPI_CALL driver_GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,\n'
        common_src += '                                                              VkPhysicalDeviceProperties* pProperties) {\n'
        common_src += '    if (nullptr != pProperties) {\n'
        common_src += '        pProperties->apiVersion = VK_API_VERSION_1_3;\n'
        common_src += '        pProperties->driverVersion = VK_API_VERSION_1_3;\n'
        common_src += '        pProperties->vendorID = 0xFEEDF00D;\n'
        common_src += '        pProperties->deviceID = 1;\n'
        common_src += '        pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;\n'
        common_src += '#if defined(_WIN32)\n'
        common_src += '        strncpy_s(pProperties->deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, driver_phys_device_name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);\n'
        common_src += '#else\n'
        common_src += '        strncpy(pProperties->deviceName, driver_phys_device_name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);\n'
        common_src += '#endif\n'
        common_src += '    }\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR void VKAPI_CALL driver_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,\n'
        common_src += '                                                               VkPhysicalDeviceProperties2* pProperties) {\n'
        common_src += '    if (nullptr != pProperties) {\n'
        common_src += '        driver_GetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);\n'
        common_src += '        VkBaseInStructure* pNext = reinterpret_cast<VkBaseInStructure*>(pProperties->pNext);\n'
        common_src += '        while (pNext) {\n'
        common_src += '            if (pNext->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT) {\n'
        common_src += '                VkPhysicalDevicePCIBusInfoPropertiesEXT* bus_info =\n'
        common_src += '                    reinterpret_cast<VkPhysicalDevicePCIBusInfoPropertiesEXT*>(pNext);\n'
        common_src += '                bus_info->pciBus = rand() % 5;\n'
        common_src += '            }\n'
        common_src += '            pNext = reinterpret_cast<VkBaseInStructure*>(const_cast<VkBaseInStructure*>(pNext->pNext));\n'
        common_src += '        }\n'
        common_src += '    }\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_CreateDebugUtilsMessengerEXT(VkInstance instance,\n'
        common_src += '                                                                   const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,\n'
        common_src += '                                                                   const VkAllocationCallbacks* pAllocator,\n'
        common_src += '                                                                   VkDebugUtilsMessengerEXT* pMessenger) {\n'
        common_src += '    *pMessenger = reinterpret_cast<VkDebugUtilsMessengerEXT>(reinterpret_cast<VkDebugUtilsMessengerEXT*>(0xdeadbeefdeadbeef));\n'
        common_src += '    driver.debug_util_info.severities = pCreateInfo->messageSeverity;\n'
        common_src += '    driver.debug_util_info.types = pCreateInfo->messageType;\n'
        common_src += '    driver.debug_util_info.callback = pCreateInfo->pfnUserCallback;\n'
        common_src += '    driver.debug_util_info.user_data = pCreateInfo->pUserData;\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += '\n'
        common_src += '//// Physical Device functions ////\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount,\n'
        common_src += '                                                                     VkLayerProperties* pProperties) {\n'
        common_src += '    log_driver_message("Generated Driver vkEnumerateDeviceLayerProperties");\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName,\n'
        common_src += '                                                                         uint32_t* pPropertyCount,\n'
        common_src += '                                                                         VkExtensionProperties* pProperties) {\n'
        common_src += '    log_driver_message("Generated Driver vkEnumerateDeviceExtensionProperties");\n'
        common_src += '    driver_initialize_device_exts();\n'
        common_src += '\n'
        common_src += '    VkResult res = VK_SUCCESS;\n'
        common_src += '    if (pLayerName == nullptr) {\n'
        common_src += '        if (pProperties != nullptr) {\n'
        common_src += '            uint32_t count = static_cast<uint32_t>(driver.device_extensions.size());\n'
        common_src += '            if (*pPropertyCount < count) {\n'
        common_src += '                count = *pPropertyCount;\n'
        common_src += '                res = VK_INCOMPLETE;\n'
        common_src += '            }\n'
        common_src += '            for (uint32_t ext = 0; ext < count; ++ext) {\n'
        common_src += '                pProperties[ext].specVersion = 1;\n'
        common_src += '#if defined(_WIN32)\n'
        common_src += '            strncpy_s(pProperties[ext].extensionName, VK_MAX_EXTENSION_NAME_SIZE, driver.device_extensions[ext].c_str(), driver.device_extensions[ext].size() + 1);\n'
        common_src += '#else\n'
        common_src += '            strncpy(pProperties[ext].extensionName, driver.device_extensions[ext].c_str(), VK_MAX_EXTENSION_NAME_SIZE);\n'
        common_src += '#endif\n'
        common_src += '            }\n'
        common_src += '        } else {\n'
        common_src += '            *pPropertyCount = static_cast<uint32_t>(driver.device_extensions.size());\n'
        common_src += '        }\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    return res;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR VkResult VKAPI_CALL driver_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,\n'
        common_src += '                                                   const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {\n'
        common_src += '    log_driver_message("Generated Driver vkCreateDevice");\n'
        common_src += '    driver_initialize_device_exts();\n'
        common_src += '\n'
        common_src += '    DispatchableHandle<VkDevice>* temp_handle = new DispatchableHandle<VkDevice>();\n'
        common_src += '    driver.dev_handles.push_back(temp_handle);\n'
        common_src += '    *pDevice = temp_handle->handle;\n'
        common_src += '\n'
        common_src += '    for (uint32_t ext = 0; ext < pCreateInfo->enabledExtensionCount; ++ext) {\n'
        common_src += '        driver.enabled_device_extensions.push_back(pCreateInfo->ppEnabledExtensionNames[ext]);\n'
        common_src += '    }\n'
        common_src += '\n'
        common_src += '    return VK_SUCCESS;\n'
        common_src += '}\n'
        common_src += '\n'
        common_src += 'VKAPI_ATTR void VKAPI_CALL driver_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {\n'
        common_src += '    log_driver_message("Generated Driver vkDestroyDevice");\n'
        common_src += '    for (uint32_t ii = 0; ii < static_cast<uint32_t>(driver.dev_handles.size()); ++ii) {\n'
        common_src += '        if (driver.dev_handles[ii]->handle == device) {\n'
        common_src += '            delete driver.dev_handles[ii];\n'
        common_src += '            driver.dev_handles.erase(driver.dev_handles.begin() + ii);\n'
        common_src += '        }\n'
        common_src += '    }\n'
        common_src += '    if (driver.dev_handles.size() == 0) {\n'
        for handle in self.handles.values():
            if handle.is_dispatchable and handle.name not in ['VkInstance', 'VkPhysicalDevice', 'VkDevice']:
                handle_vect_name = 'driver.%s_handles' % handle.name[2:].lower()
                common_src += '        for (uint32_t ii = 0; ii < static_cast<uint32_t>(%s.size()); ++ii) {\n' % handle_vect_name
                common_src += '            delete %s[ii];\n' % handle_vect_name
                common_src += '        }\n'
                common_src += '        %s.clear();\n' % handle_vect_name
        common_src += '    }\n'
        common_src += '}\n'
        return common_src

    def OutputDriverGIPA(self):
        driver_gipa = ''
        driver_gipa += '\n'
        driver_gipa += 'bool IsInstanceExtensionEnabled(const char* extension_name) {\n'
        driver_gipa += '    return driver.enabled_instance_extensions.end() !=\n'
        driver_gipa += '           std::find_if(driver.enabled_instance_extensions.begin(), driver.enabled_instance_extensions.end(),\n'
        driver_gipa += '                        [extension_name](Extension const& ext) { return ext.extensionName == extension_name; });\n'
        driver_gipa += '}\n'
        driver_gipa += '\n'
        driver_gipa += '// Prototypes:\n'
        driver_gipa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL driver_GetDeviceProcAddr(VkDevice device, const char* pName);\n'
        driver_gipa += '\n'
        driver_gipa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL driver_GetInstanceProcAddr(VkInstance instance, const char* pName) {\n'

        printed_commands = set()
        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            driver_gipa += '\n    // ----- %s\n' % ext.name

            if (ext.protect is not None):
                driver_gipa += '#ifdef %s\n' % ext.protect

            major = 1
            minor = 0
            add_indent = ''
            if 'VK_VERSION_' in ext.name:
                version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
                major = version_numbers[0]
                minor = version_numbers[1]

                driver_gipa += '    {\n'
                add_indent = '    '
            elif ext.type == 'instance':
                driver_gipa += '    if (IsInstanceExtensionEnabled("%s")) {\n' % ext.name
                add_indent = '    '

            for cmd_name in ext.command_data.keys():
                if cmd_name in printed_commands:
                    continue
                driver_gipa += '    %sif (string_eq(pName, "%s")) return to_vkVoidFunction(driver_%s);\n' % (add_indent, cmd_name, cmd_name[2:])
                printed_commands.add(cmd_name)

            if 'VK_VERSION_' in ext.name:
                driver_gipa += '    } '
                driver_gipa += '// End Core %d.%d\n' % (major, minor)
            elif ext.type == 'instance':
                driver_gipa += '    } // %s\n' % ext.name

            if (ext.protect is not None):
                driver_gipa += '#endif // %s\n' % ext.protect

        driver_gipa += '\n'
        driver_gipa += '    return nullptr;\n'
        driver_gipa += '}\n'
        driver_gipa += '\n'
        return driver_gipa

    def OutputDriverGDPA(self):
        driver_gdpa = ''
        driver_gdpa += 'bool IsDeviceExtensionEnabled(const char* extension_name) {\n'
        driver_gdpa += '    return driver.enabled_device_extensions.end() !=\n'
        driver_gdpa += '           std::find_if(driver.enabled_device_extensions.begin(),\n'
        driver_gdpa += '                        driver.enabled_device_extensions.end(),\n'
        driver_gdpa += '                        [extension_name](Extension const& ext) { return ext.extensionName == extension_name; });\n'
        driver_gdpa += '}\n\n'
        driver_gdpa += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL driver_GetDeviceProcAddr(VkDevice device, const char* pName) {\n'
        printed_commands = set()

        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            driver_gdpa += '\n    // ----- %s\n' % ext.name

            if (ext.protect is not None):
                driver_gdpa += '#ifdef %s\n' % ext.protect

            major = 1
            minor = 0
            add_indent = ''
            if 'VK_VERSION_' in ext.name:
                version_numbers = list(map(int, re.findall(r'\d+', ext.name)))
                major = version_numbers[0]
                minor = version_numbers[1]

                driver_gdpa += '    {\n'
                add_indent = '    '
            elif ext.type == 'device':
                driver_gdpa += '    if (IsDeviceExtensionEnabled("%s")) {\n' % ext.name
                add_indent = '    '

            for cmd_name in ext.command_data.keys():
                if cmd_name in printed_commands:
                    continue
                driver_gdpa += '    %sif (string_eq(pName, "%s")) return to_vkVoidFunction(driver_%s);\n' % (add_indent, cmd_name, cmd_name[2:])
                printed_commands.add(cmd_name)

            if 'VK_VERSION_' in ext.name:
                driver_gdpa += '    } '
                driver_gdpa += '// End Core %d.%d\n' % (major, minor)
            elif ext.type == 'device':
                driver_gdpa += '    } // %s\n' % ext.name

            if (ext.protect is not None):
                driver_gdpa += '#endif // %s\n' % ext.protect

        driver_gdpa += '\n'
        driver_gdpa += '    return nullptr;\n'
        driver_gdpa += '}\n'
        driver_gdpa += '\n'
        return driver_gdpa

    def OutputDriverFunctionExport(self):
        driver_wrapup = ''
        driver_wrapup += '\n'
        driver_wrapup += '// Exported functions\n'
        driver_wrapup += 'extern "C" {\n'
        driver_wrapup += '\n'
        driver_wrapup += self.OutputDriverGIPA()
        driver_wrapup += self.OutputDriverGDPA()
        driver_wrapup += 'extern FRAMEWORK_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion) {\n'
        driver_wrapup += '    *pSupportedVersion = driver.max_icd_interface_version;\n'
        driver_wrapup += '    return VK_SUCCESS;\n'
        driver_wrapup += '}\n'
        driver_wrapup += '\n'
        driver_wrapup += 'FRAMEWORK_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {\n'
        driver_wrapup += '    return driver_GetInstanceProcAddr(instance, pName);\n'
        driver_wrapup += '}\n'
        driver_wrapup += '}  // extern "C"\n'
        driver_wrapup += '\n'
        return driver_wrapup

    def PrintDriverCommand(self, command_name):
        cmd = self.commands.get(command_name)
        if cmd is None:
            return
        cmd_str = ''
        cmd_str += cmd.cdecl.replace(';', ' {\n').replace('VKAPI_CALL vk', 'VKAPI_CALL driver_')
        cmd_str += '    log_driver_message("Generated Driver %s");\n' % cmd.name
        if cmd.alias:
            cmd_str += '    return driver_%s(' %cmd.alias[2:]
            param_list = []
            for param in cmd.params:
                param_list.append(param.name)
            cmd_str += ', '.join(param_list)
            cmd_str += ');\n'
        else:
            if cmd.is_create:
                handle = self.handles.get(cmd.modified_handle)
                if handle is not None:
                    if handle.is_dispatchable:
                        cmd_str += '    DispatchableHandle<%s>* temp_handle = new DispatchableHandle<%s>();\n' % (cmd.modified_handle, cmd.modified_handle)
                        cmd_str += '    driver.%s_handles.push_back(temp_handle);\n' % cmd.modified_handle[2:].lower()
                        cmd_str += '    *%s = temp_handle->handle;\n' % cmd.params[-1].name
                    else:
                        cmd_str += '    *%s = reinterpret_cast<%s>(reinterpret_cast<%s*>(0xdeadbeefdeadbeef));\n' % (cmd.params[-1].name, cmd.params[-1].type, cmd.params[-1].type)
            elif cmd.is_destroy:
                handle = self.handles.get(cmd.modified_handle)
                if handle is not None and handle.is_dispatchable:
                    handle_vect_name = 'driver.%s_handles' % cmd.modified_handle[2:].lower()
                    param_name = cmd.params[-1].name
                    if cmd.params[-1].is_pointer:
                        param_name = '*' + param_name
                    cmd_str += '    for (uint32_t ii = 0; ii < static_cast<uint32_t>(%s.size()); ++ii) {\n' % handle_vect_name
                    cmd_str += '        if (%s[ii]->handle == %s) {\n' % (handle_vect_name, param_name)
                    cmd_str += '            delete %s[ii];\n' % handle_vect_name
                    cmd_str += '            %s.erase(%s.begin() + ii);\n' % (handle_vect_name, handle_vect_name)
                    cmd_str += '        }\n'
                    cmd_str += '    }\n'

            if cmd.return_type is not None:
                if 'VkResult' == cmd.return_type.text:
                    cmd_str += '    return VK_SUCCESS;\n'
                elif '*' in cmd.return_type.text:
                    cmd_str += '    return nullptr;\n'
                elif ('int' in cmd.return_type.text or
                    'VkDeviceAddress' == cmd.return_type.text or
                    'VkDeviceSize' ==  cmd.return_type.text):
                    cmd_str += '    return static_cast<%s>(0);\n' % cmd.return_type.text
                elif 'VkBool32' == cmd.return_type.text:
                    cmd_str += '    return VK_TRUE;\n'
                else:
                    cmd_str += '    return TODO!;\n'
        cmd_str += '}\n'
        return cmd_str

    def GenerateDriverSource(self):
        driver_src = ''
        driver_src += 'EntrypointTestDriver driver;\n'
        driver_src += 'extern "C" {\n'
        driver_src += 'FRAMEWORK_EXPORT EntrypointTestDriver* get_ep_driver_func() { return &driver; }\n'
        driver_src += 'FRAMEWORK_EXPORT EntrypointTestDriver* reset_ep_driver_func() {\n'
        driver_src += '    driver.~EntrypointTestDriver();\n'
        driver_src += '    return new (&driver) EntrypointTestDriver();\n'
        driver_src += '}\n'
        driver_src += '}\n'
        driver_src += '\n'

        driver_src += self.OutputDriverAliasPrototypes()
        driver_src += self.OutputCommonInstanceDeviceDriverFuncs()

        printed_commands = set()
        # Printout funcs
        for ext in self.extensions.values():
            if len(ext.command_data) == 0:
                continue

            driver_src += '\n'
            driver_src += '// ----- %s\n' % ext.name

            if (ext.protect is not None):
                driver_src += '#ifdef %s\n' % ext.protect

            for cmd_name in ext.command_data.keys():
                if (cmd_name in MANUAL_LAYER_DRIVER_ENTRYPOINTS or
                    cmd_name in MANUAL_DRIVER_ENTRYPOINTS or
                    cmd_name in printed_commands):
                    continue

                driver_src += self.PrintDriverCommand(cmd_name)
                driver_src += '\n'
                printed_commands.add(cmd_name)

            if (ext.protect is not None):
                driver_src += '#endif // %s\n' % ext.protect


        driver_src += self.OutputDriverFunctionExport()

        driver_src += '\n'
        return driver_src

class BasicHandleData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')             # The name of the handle
        self.alias = kwargs.get('alias')            # Any alias for this handle
        self.is_dispatchable = kwargs.get('is_dispatchable')  # Boolean for is this handle a dispatchable type

class HandleTreeData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')           # Name of type used for handle (or None)
        self.alias = kwargs.get('alias')            # Name of alias of this handle (or None)
        self.parents = kwargs.get('parents')          # Name of parent handles
        self.children = kwargs.get('children')         # Name of children handles
        self.create_funcs = kwargs.get('create_funcs')     # BasicCommandData tuple list of functions used to create this handle
        self.destroy_funcs = kwargs.get('destroy_funcs')    # BasicCommandData tuple list of functions used to destroy this handle
        self.usage_funcs = kwargs.get('usage_funcs')      # BasicCommandData tuple list of functions that use this handle

class BasicCommandData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')             # Name of the command
        self.protect = kwargs.get('protect')          # Any protected info (i.e. if this requires an #ifdef)
        self.return_type = kwargs.get('return_type')      # The returned type
        self.raw_handle = kwargs.get('raw_handle')       # The raw handle data
        self.handle_type = kwargs.get('handle_type')      # The string handle type (i.e. 'VkInstance',...)
        self.is_create = kwargs.get('is_create')        # Boolean for is this a create command
        self.is_destroy = kwargs.get('is_destroy')       # Boolean for is this a destroy command
        self.is_begin = kwargs.get('is_begin')         # Boolean for is this a begin command
        self.is_end = kwargs.get('is_end')           # Boolean for is this an end command
        self.modified_handle = kwargs.get('modified_handle')  # If this is a create or destroy command, what handle does it modify
        self.handles_used = kwargs.get('handles_used')     # List of all handles used in this command (excluding the modified handle)
        self.params = kwargs.get('params')           # List of all params (as CommandParam tuples)
        self.cdecl = kwargs.get('cdecl')            # The raw cdecl for this command
        self.alias = kwargs.get('alias')            # Any alias name for this command (i.e. it was promoted to core or a KHR extension)

class CommandParam :
    def __init__(self, **kwargs):
        self.type = kwargs.get('type')             # Data type of the command parameter
        self.name = kwargs.get('name')             # Name of the parameter
        self.length_info = kwargs.get('length_info')      # Name of a length item that adjust this parameter
        self.is_const = kwargs.get('is_const')         # Boolean for is this a constant param
        self.is_pointer = kwargs.get('is_pointer')       # Boolean for is this a pointer param
        self.is_array = kwargs.get('is_array')         # Boolean for is this a array param
        self.array_1st_size = kwargs.get('array_1st_size')   # Size of first array dimension (only valid if is_array = 'True')
        self.array_2nd_size = kwargs.get('array_2nd_size')   # Size of second array dimension (only valid if is_array = 'True' and may be 0 if 1d array)
        self.cdecl = kwargs.get('cdecl')            # The raw cdecl for this parameter

class BasicExtensionData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')             # The name of the extension
        self.type = kwargs.get('type')             # The type ('instance' or 'device') of the extension
        self.protect = kwargs.get('protect')          # Any protected info (i.e. if this requires an #ifdef)
        self.define_name = kwargs.get('define_name')      # Name of #define used for the extension name
        self.required_exts = kwargs.get('required_exts')    # Dict of any additional extensions required to use this extension, use dict as a set with None for the values
        self.command_data = kwargs.get('command_data')     # BasicExtensionCommandData tuple list of all commands

class BasicExtensionCommandData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')             # Name of command
        self.requires = kwargs.get('requires')         # Any additional extensions this one command requires

class TestVariableNames :
    def __init__(self, **kwargs):
        self.type =kwargs.get('type')             # Type of test variable
        self.name =kwargs.get('name')             # Name of test variable
        self.is_array =kwargs.get('is_array')         # Boolean for is this a array param
        self.array_1st_size =kwargs.get('array_1st_size')   # Size of first array dimension (only valid if is_array = 'True')
        self.array_2nd_size =kwargs.get('array_2nd_size')   # Size of second array dimension (only valid if is_array = 'True' and may be 0 if 1d array)

class StructData :
    def __init__(self, **kwargs):
        self.name = kwargs.get('name')             # Type name of test struct
        self.protect = kwargs.get('protect')          # Any protected info (i.e. if this requires an #ifdef)
        self.members = kwargs.get('members')          # StructMember list of members

class StructMember :
    def __init__(self, **kwargs):
        self.type = kwargs.get('type')             # Data type of the struct member
        self.name = kwargs.get('name')             # Name of the struct member
        self.req_value = kwargs.get('req_value')        # Required value
        self.cdecl = kwargs.get('cdecl')            # The raw cdecl
