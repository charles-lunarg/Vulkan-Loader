/*
 *
 * Copyright (c) 2014-2021 The Khronos Group Inc.
 * Copyright (c) 2014-2021 Valve Corporation
 * Copyright (c) 2014-2021 LunarG, Inc.
 * Copyright (C) 2015 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 *
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Young <marky@lunarg.com>
 * Author: Lenny Komow <lenny@lunarg.com>
 * Author: Charles Giessen <charles@lunarg.com>
 *
 */

// Terminators which have simple logic belong here, since they are mostly "pass through"
// Function declarations are in vk_loader_terminators.h, thus not needed here

#include "allocation.h"
#include "loader_common.h"
#include "loader.h"
#include "log.h"
#include "debug_utils.h"
#include "extension_manual.h"
#include "vk_loader_platform.h"
#include "wsi.h"
#include <vulkan/vk_icd.h>

// Terminators for 1.0 functions

VKAPI_ATTR VkResult VKAPI_CALL terminator_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                                         VkLayerProperties *pProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    loader_log(icd_term->this_instance, VULKAN_LOADER_ERROR_BIT, 0,
               "Encountered the vkEnumerateDeviceLayerProperties terminator.  This means a layer improperly continued.");
    // Should never get here this call isn't dispatched down the chain
    return VK_ERROR_INITIALIZATION_FAILED;
}

// Terminators for 1.1 functions

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
                                                                 VkPhysicalDeviceFeatures2 *pFeatures) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceFeatures2 fpGetPhysicalDeviceFeatures2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceFeatures2 = icd_term->dispatch.GetPhysicalDeviceFeatures2;
    }
    if (fpGetPhysicalDeviceFeatures2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceFeatures2 = icd_term->dispatch.GetPhysicalDeviceFeatures2KHR;
    }

    if (fpGetPhysicalDeviceFeatures2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceFeatures2(phys_dev_term->phys_dev, pFeatures);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceFeatures2: Emulating call for driver \"%s\" device \"%s\" using vkGetPhysicalDeviceFeatures",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        // Write to the VkPhysicalDeviceFeatures2 struct
        icd_term->dispatch.GetPhysicalDeviceFeatures(phys_dev_term->phys_dev, &pFeatures->features);

        const VkBaseInStructure *pNext = pFeatures->pNext;
        while (pNext != NULL) {
            switch (pNext->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES: {
                    // Skip the check if VK_KHR_multiview is enabled because it's a device extension
                    // Write to the VkPhysicalDeviceMultiviewFeaturesKHR struct
                    VkPhysicalDeviceMultiviewFeaturesKHR *multiview_features = (VkPhysicalDeviceMultiviewFeaturesKHR *)pNext;
                    multiview_features->multiview = VK_FALSE;
                    multiview_features->multiviewGeometryShader = VK_FALSE;
                    multiview_features->multiviewTessellationShader = VK_FALSE;

                    pNext = multiview_features->pNext;
                    break;
                }
                default: {
                    loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                               "vkGetPhysicalDeviceFeatures2: Emulation found unrecognized structure type in pFeatures->pNext - "
                               "this struct will be ignored");

                    pNext = pNext->pNext;
                    break;
                }
            }
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
                                                                   VkPhysicalDeviceProperties2 *pProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceProperties2 = icd_term->dispatch.GetPhysicalDeviceProperties2;
    }
    if (fpGetPhysicalDeviceProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceProperties2 = icd_term->dispatch.GetPhysicalDeviceProperties2KHR;
    }

    if (fpGetPhysicalDeviceProperties2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceProperties2(phys_dev_term->phys_dev, pProperties);
    } else {
        // Emulate the call
        loader_log(
            icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
            "vkGetPhysicalDeviceProperties2: Emulating call for driver \"%s\" device \"%s\" using vkGetPhysicalDeviceProperties",
            icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        // Write to the VkPhysicalDeviceProperties2 struct
        icd_term->dispatch.GetPhysicalDeviceProperties(phys_dev_term->phys_dev, &pProperties->properties);

        const VkBaseInStructure *pNext = pProperties->pNext;
        while (pNext != NULL) {
            switch (pNext->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES: {
                    VkPhysicalDeviceIDPropertiesKHR *id_properties = (VkPhysicalDeviceIDPropertiesKHR *)pNext;

                    // Verify that "VK_KHR_external_memory_capabilities" is enabled
                    if (icd_term->this_instance->inst_ext_enables.khr_external_memory_capabilities) {
                        loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                                   "vkGetPhysicalDeviceProperties2: Emulation cannot generate unique IDs for struct "
                                   "VkPhysicalDeviceIDProperties - setting IDs to zero instead");

                        // Write to the VkPhysicalDeviceIDPropertiesKHR struct
                        memset(id_properties->deviceUUID, 0, VK_UUID_SIZE);
                        memset(id_properties->driverUUID, 0, VK_UUID_SIZE);
                        id_properties->deviceLUIDValid = VK_FALSE;
                    }

                    pNext = id_properties->pNext;
                    break;
                }
                default: {
                    loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                               "vkGetPhysicalDeviceProperties2KHR: Emulation found unrecognized structure type in "
                               "pProperties->pNext - this struct will be ignored");

                    pNext = pNext->pNext;
                    break;
                }
            }
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                         VkFormatProperties2 *pFormatProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceFormatProperties2 fpGetPhysicalDeviceFormatProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceFormatProperties2;
    }
    if (fpGetPhysicalDeviceFormatProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceFormatProperties2KHR;
    }

    if (fpGetPhysicalDeviceFormatProperties2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceFormatProperties2(phys_dev_term->phys_dev, format, pFormatProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceFormatProperties2: Emulating call for driver \"%s\" device \"%s\" using "
                   "vkGetPhysicalDeviceFormatProperties",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        // Write to the VkFormatProperties2 struct
        icd_term->dispatch.GetPhysicalDeviceFormatProperties(phys_dev_term->phys_dev, format, &pFormatProperties->formatProperties);

        if (pFormatProperties->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceFormatProperties2: Emulation found unrecognized structure type in "
                       "pFormatProperties->pNext - this struct will be ignored");
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetPhysicalDeviceImageFormatProperties2(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2KHR *pImageFormatInfo,
    VkImageFormatProperties2KHR *pImageFormatProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceImageFormatProperties2 fpGetPhysicalDeviceImageFormatProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceImageFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceImageFormatProperties2;
    }
    if (fpGetPhysicalDeviceImageFormatProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceImageFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceImageFormatProperties2KHR;
    }

    if (fpGetPhysicalDeviceImageFormatProperties2 != NULL) {
        // Pass the call to the driver
        return fpGetPhysicalDeviceImageFormatProperties2(phys_dev_term->phys_dev, pImageFormatInfo, pImageFormatProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceImageFormatProperties2: Emulating call for driver \"%s\" device \"%s\" using "
                   "vkGetPhysicalDeviceImageFormatProperties",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        // If there is more info in  either pNext, then this is unsupported
        if (pImageFormatInfo->pNext != NULL || pImageFormatProperties->pNext != NULL) {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }

        // Write to the VkImageFormatProperties2KHR struct
        return icd_term->dispatch.GetPhysicalDeviceImageFormatProperties(
            phys_dev_term->phys_dev, pImageFormatInfo->format, pImageFormatInfo->type, pImageFormatInfo->tiling,
            pImageFormatInfo->usage, pImageFormatInfo->flags, &pImageFormatProperties->imageFormatProperties);
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
                                                                              uint32_t *pQueueFamilyPropertyCount,
                                                                              VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 fpGetPhysicalDeviceQueueFamilyProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceQueueFamilyProperties2 = icd_term->dispatch.GetPhysicalDeviceQueueFamilyProperties2;
    }
    if (fpGetPhysicalDeviceQueueFamilyProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceQueueFamilyProperties2 = icd_term->dispatch.GetPhysicalDeviceQueueFamilyProperties2KHR;
    }

    if (fpGetPhysicalDeviceQueueFamilyProperties2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceQueueFamilyProperties2(phys_dev_term->phys_dev, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceQueueFamilyProperties2: Emulating call for driver \"%s\" device \"%s\" using "
                   "vkGetPhysicalDeviceQueueFamilyProperties",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        if (pQueueFamilyProperties == NULL || *pQueueFamilyPropertyCount == 0) {
            // Write to pQueueFamilyPropertyCount
            icd_term->dispatch.GetPhysicalDeviceQueueFamilyProperties(phys_dev_term->phys_dev, pQueueFamilyPropertyCount, NULL);
        } else {
            // Allocate a temporary array for the output of the old function
            VkQueueFamilyProperties *properties = loader_stack_alloc(*pQueueFamilyPropertyCount * sizeof(VkQueueFamilyProperties));
            if (properties == NULL) {
                *pQueueFamilyPropertyCount = 0;
                loader_log(icd_term->this_instance, VULKAN_LOADER_ERROR_BIT, 0,
                           "vkGetPhysicalDeviceQueueFamilyProperties2: Out of memory - Failed to allocate array for loader "
                           "emulation.");
                return;
            }

            icd_term->dispatch.GetPhysicalDeviceQueueFamilyProperties(phys_dev_term->phys_dev, pQueueFamilyPropertyCount,
                                                                      properties);
            for (uint32_t i = 0; i < *pQueueFamilyPropertyCount; ++i) {
                // Write to the VkQueueFamilyProperties2KHR struct
                memcpy(&pQueueFamilyProperties[i].queueFamilyProperties, &properties[i], sizeof(VkQueueFamilyProperties));

                if (pQueueFamilyProperties[i].pNext != NULL) {
                    loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                               "vkGetPhysicalDeviceQueueFamilyProperties2: Emulation found unrecognized structure type in "
                               "pQueueFamilyProperties[%d].pNext - this struct will be ignored",
                               i);
                }
            }
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
                                                                         VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceMemoryProperties2 fpGetPhysicalDeviceMemoryProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceMemoryProperties2 = icd_term->dispatch.GetPhysicalDeviceMemoryProperties2;
    }
    if (fpGetPhysicalDeviceMemoryProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceMemoryProperties2 = icd_term->dispatch.GetPhysicalDeviceMemoryProperties2KHR;
    }

    if (fpGetPhysicalDeviceMemoryProperties2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceMemoryProperties2(phys_dev_term->phys_dev, pMemoryProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceMemoryProperties2: Emulating call for driver \"%s\" device \"%s\" using "
                   "vkGetPhysicalDeviceMemoryProperties",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        // Write to the VkPhysicalDeviceMemoryProperties2 struct
        icd_term->dispatch.GetPhysicalDeviceMemoryProperties(phys_dev_term->phys_dev, &pMemoryProperties->memoryProperties);

        if (pMemoryProperties->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceMemoryProperties2: Emulation found unrecognized structure type in "
                       "pMemoryProperties->pNext - this struct will be ignored");
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceSparseImageFormatProperties2(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2KHR *pFormatInfo, uint32_t *pPropertyCount,
    VkSparseImageFormatProperties2KHR *pProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 fpGetPhysicalDeviceSparseImageFormatProperties2 = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceSparseImageFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceSparseImageFormatProperties2;
    }
    if (fpGetPhysicalDeviceSparseImageFormatProperties2 == NULL && inst->inst_ext_enables.khr_get_physical_device_properties2) {
        fpGetPhysicalDeviceSparseImageFormatProperties2 = icd_term->dispatch.GetPhysicalDeviceSparseImageFormatProperties2KHR;
    }

    if (fpGetPhysicalDeviceSparseImageFormatProperties2 != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceSparseImageFormatProperties2(phys_dev_term->phys_dev, pFormatInfo, pPropertyCount, pProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceSparseImageFormatProperties2: Emulating call for driver \"%s\" device \"%s\" using "
                   "vkGetPhysicalDeviceSparseImageFormatProperties",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        if (pFormatInfo->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceSparseImageFormatProperties2: Emulation found unrecognized structure type in "
                       "pFormatInfo->pNext - this struct will be ignored");
        }

        if (pProperties == NULL || *pPropertyCount == 0) {
            // Write to pPropertyCount
            icd_term->dispatch.GetPhysicalDeviceSparseImageFormatProperties(
                phys_dev_term->phys_dev, pFormatInfo->format, pFormatInfo->type, pFormatInfo->samples, pFormatInfo->usage,
                pFormatInfo->tiling, pPropertyCount, NULL);
        } else {
            // Allocate a temporary array for the output of the old function
            VkSparseImageFormatProperties *properties =
                loader_stack_alloc(*pPropertyCount * sizeof(VkSparseImageMemoryRequirements));
            if (properties == NULL) {
                *pPropertyCount = 0;
                loader_log(icd_term->this_instance, VULKAN_LOADER_ERROR_BIT, 0,
                           "vkGetPhysicalDeviceSparseImageFormatProperties2: Out of memory - Failed to allocate array for "
                           "loader emulation.");
                return;
            }

            icd_term->dispatch.GetPhysicalDeviceSparseImageFormatProperties(
                phys_dev_term->phys_dev, pFormatInfo->format, pFormatInfo->type, pFormatInfo->samples, pFormatInfo->usage,
                pFormatInfo->tiling, pPropertyCount, properties);
            for (uint32_t i = 0; i < *pPropertyCount; ++i) {
                // Write to the VkSparseImageFormatProperties2KHR struct
                memcpy(&pProperties[i].properties, &properties[i], sizeof(VkSparseImageFormatProperties));

                if (pProperties[i].pNext != NULL) {
                    loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                               "vkGetPhysicalDeviceSparseImageFormatProperties2: Emulation found unrecognized structure type in "
                               "pProperties[%d].pNext - this struct will be ignored",
                               i);
                }
            }
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceExternalBufferProperties(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
    VkExternalBufferProperties *pExternalBufferProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceExternalBufferProperties fpGetPhysicalDeviceExternalBufferProperties = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceExternalBufferProperties = icd_term->dispatch.GetPhysicalDeviceExternalBufferProperties;
    }
    if (fpGetPhysicalDeviceExternalBufferProperties == NULL && inst->inst_ext_enables.khr_external_memory_capabilities) {
        fpGetPhysicalDeviceExternalBufferProperties = icd_term->dispatch.GetPhysicalDeviceExternalBufferPropertiesKHR;
    }

    if (fpGetPhysicalDeviceExternalBufferProperties != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceExternalBufferProperties(phys_dev_term->phys_dev, pExternalBufferInfo, pExternalBufferProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceExternalBufferProperties: Emulating call for driver \"%s\" device \"%s\"",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        if (pExternalBufferInfo->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalBufferProperties: Emulation found unrecognized structure type in "
                       "pExternalBufferInfo->pNext - this struct will be ignored");
        }

        // Fill in everything being unsupported
        memset(&pExternalBufferProperties->externalMemoryProperties, 0, sizeof(VkExternalMemoryPropertiesKHR));

        if (pExternalBufferProperties->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalBufferProperties: Emulation found unrecognized structure type in "
                       "pExternalBufferProperties->pNext - this struct will be ignored");
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceExternalSemaphoreProperties(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties *pExternalSemaphoreProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties fpGetPhysicalDeviceExternalSemaphoreProperties = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceExternalSemaphoreProperties = icd_term->dispatch.GetPhysicalDeviceExternalSemaphoreProperties;
    }
    if (fpGetPhysicalDeviceExternalSemaphoreProperties == NULL && inst->inst_ext_enables.khr_external_semaphore_capabilities) {
        fpGetPhysicalDeviceExternalSemaphoreProperties = icd_term->dispatch.GetPhysicalDeviceExternalSemaphorePropertiesKHR;
    }

    if (fpGetPhysicalDeviceExternalSemaphoreProperties != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceExternalSemaphoreProperties(phys_dev_term->phys_dev, pExternalSemaphoreInfo,
                                                       pExternalSemaphoreProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceExternalSemaphoreProperties: Emulating call for driver \"%s\" device \"%s\"",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        if (pExternalSemaphoreInfo->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalSemaphoreProperties: Emulation found unrecognized structure type in "
                       "pExternalSemaphoreInfo->pNext - this struct will be ignored");
        }

        // Fill in everything being unsupported
        pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
        pExternalSemaphoreProperties->compatibleHandleTypes = 0;
        pExternalSemaphoreProperties->externalSemaphoreFeatures = 0;

        if (pExternalSemaphoreProperties->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalSemaphoreProperties: Emulation found unrecognized structure type in "
                       "pExternalSemaphoreProperties->pNext - this struct will be ignored");
        }
    }
}

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceExternalFenceProperties(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
    VkExternalFenceProperties *pExternalFenceProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    const struct loader_instance *inst = icd_term->this_instance;

    assert(inst != NULL);

    // Get the function pointer to use to call into the driver. This could be the core or KHR version
    PFN_vkGetPhysicalDeviceExternalFenceProperties fpGetPhysicalDeviceExternalFenceProperties = NULL;
    if (loader_check_version_meets_required(LOADER_VERSION_1_1_0, inst->app_api_version)) {
        fpGetPhysicalDeviceExternalFenceProperties = icd_term->dispatch.GetPhysicalDeviceExternalFenceProperties;
    }
    if (fpGetPhysicalDeviceExternalFenceProperties == NULL && inst->inst_ext_enables.khr_external_fence_capabilities) {
        fpGetPhysicalDeviceExternalFenceProperties = icd_term->dispatch.GetPhysicalDeviceExternalFencePropertiesKHR;
    }

    if (fpGetPhysicalDeviceExternalFenceProperties != NULL) {
        // Pass the call to the driver
        fpGetPhysicalDeviceExternalFenceProperties(phys_dev_term->phys_dev, pExternalFenceInfo, pExternalFenceProperties);
    } else {
        // Emulate the call
        loader_log(icd_term->this_instance, VULKAN_LOADER_INFO_BIT, 0,
                   "vkGetPhysicalDeviceExternalFenceProperties: Emulating call for driver \"%s\" device \"%s\"",
                   icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);

        if (pExternalFenceInfo->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalFenceProperties: Emulation found unrecognized structure type in "
                       "pExternalFenceInfo->pNext - this struct will be ignored");
        }

        // Fill in everything being unsupported
        pExternalFenceProperties->exportFromImportedHandleTypes = 0;
        pExternalFenceProperties->compatibleHandleTypes = 0;
        pExternalFenceProperties->externalFenceFeatures = 0;

        if (pExternalFenceProperties->pNext != NULL) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_WARN_BIT, 0,
                       "vkGetPhysicalDeviceExternalFenceProperties: Emulation found unrecognized structure type in "
                       "pExternalFenceProperties->pNext - this struct will be ignored");
        }
    }
}

// 1.3 Core terminators

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount,
                                                                          VkPhysicalDeviceToolProperties *pToolProperties) {
    struct loader_physical_device_term *phys_dev_term = (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    VkResult res = VK_SUCCESS;

    if (pToolCount != NULL && pToolProperties == NULL) {
        *pToolCount = 0;
    }

    // If this is Vulkan 1.3 or newer, then it may support the API (but then again, it may not).
    // Because of this, check for NULL before trying to call down.
    if (VK_API_VERSION_MINOR(phys_dev_term->properties.apiVersion) >= 3) {
        if (NULL == icd_term->dispatch.GetPhysicalDeviceToolProperties) {
            loader_log(icd_term->this_instance, VULKAN_LOADER_ERROR_BIT | VULKAN_LOADER_DRIVER_BIT, 0,
                       "terminator_GetPhysicalDeviceToolProperties: Driver \"%s\" device \"%s\" "
                       "vkGetPhysicalDeviceToolProperties was NULL yet device claims supports for Vulkan 1.3",
                       icd_term->scanned_icd->lib_name, phys_dev_term->properties.deviceName);
        } else {
            res = icd_term->dispatch.GetPhysicalDeviceToolProperties(phys_dev_term->phys_dev, pToolCount, pToolProperties);
            goto out;
        }
    }

    // Try using the extension version if it's available
    if (icd_term->scanned_icd->inst_ext_support.ext_tooling_info && NULL != icd_term->dispatch.GetPhysicalDeviceToolPropertiesEXT) {
        res = icd_term->dispatch.GetPhysicalDeviceToolPropertiesEXT(phys_dev_term->phys_dev, pToolCount, pToolProperties);
    }

out:
    return res;
}
