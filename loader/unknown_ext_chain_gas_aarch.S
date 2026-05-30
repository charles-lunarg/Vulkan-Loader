//
// Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// Copyright (c) 2026 Valve Corporation
// Copyright (c) 2026 LunarG, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Eric Sullivan <esullivan@nvidia.com>
// Author: Charles Giessen <charles@lunarg.com>
//

// This code is used to pass on device (including physical device) extensions through the call chain. It must do this without
// creating a stack frame, because the actual parameters of the call are not known. Since the first parameter is known to be a
// VkPhysicalDevice or a dispatchable object it can unwrap the object, possibly overwriting the wrapped physical device, and then
// jump to the next function in the call chain

#include "gen_defines.asm"

/*
 * References:
 *  - https://developer.arm.com/documentation/101028/0012/5--Feature-test-macros
 *  - https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst
 */
#if defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1
  #define BTI_J  hint 36 /* bti j: for jumps, IE br instructions */
  #define BTI_C  hint 34 /* bti c: for calls, IE bl instructions */
  #define GNU_PROPERTY_AARCH64_BTI 1 /* bit 0 GNU Notes is for BTI support */
#else
  #define BTI_J
  #define BTI_C
  #define GNU_PROPERTY_AARCH64_BTI 0
#endif

/*
 * We just need PAC added to GNU Notes for auditing features, the assembly itself does
 * not need pac augmentation at this time because it doesn't make use of the SP aka x30.
 */
#if defined(__ARM_FEATURE_PAC_DEFAULT)
  #define GNU_PROPERTY_AARCH64_POINTER_AUTH 2 /* bit 1 GNU Notes is for PAC support */
#else
  #define GNU_PROPERTY_AARCH64_POINTER_AUTH 0
#endif

.if AARCH_64

.macro DevExtTramp num
.global vkdev_ext\num
#if defined(__ELF__)
 .hidden vkdev_ext\num
#endif
.balign 4
vkdev_ext\num:
    BTI_C
    ldr     x9, [x0]                                              // Load the loader_instance_dispatch_table* into x9
    mov     x10, (EXT_OFFSET_DEVICE_DISPATCH + (PTR_SIZE * \num)) // Offset of the desired function in the dispatch table
    ldr     x16, [x9, x10]                                        // Load the function address
    br      x16
.endm

.else // AARCH_32

.macro DevExtTramp num
.global vkdev_ext\num
#if defined(__ELF__)
 .hidden vkdev_ext\num
#endif
.balign 4
vkdev_ext_dispatch_entry\num: .word EXT_OFFSET_DEVICE_DISPATCH + (PTR_SIZE * \num)
vkdev_ext\num:
    ldr     r4, [r0]                          // Load the loader_instance_dispatch_table* into r4
    ldr     r5, vkdev_ext_dispatch_entry\num // Offset of the desired function in the dispatch table
    ldr     r6, [r4, r5]                    // Load the function address
    bx      r6
.endm

.endif

#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
/* Add the PAC and BTI support to GNU Notes section for ELF object files */
#if GNU_PROPERTY_AARCH64_BTI != 0 || GNU_PROPERTY_AARCH64_POINTER_AUTH != 0
    .pushsection .note.gnu.property, "a"; /* Start a new allocatable section */
    .balign 8; /* align it on a byte boundry */
    .long 4; /* size of "GNU\0" */
    .long 0x10; /* size of descriptor */
    .long 0x5; /* NT_GNU_PROPERTY_TYPE_0 */
    .asciz "GNU";
    .long 0xc0000000; /* GNU_PROPERTY_AARCH64_FEATURE_1_AND */
    .long 4; /* Four bytes of data */
    .long (GNU_PROPERTY_AARCH64_BTI|GNU_PROPERTY_AARCH64_POINTER_AUTH); /* BTI or PAC is enabled */
    .long 0; /* padding for 8 byte alignment */
    .popsection; /* end the section */
#endif
#endif

.text

    DevExtTramp 0
    DevExtTramp 1
    DevExtTramp 2
    DevExtTramp 3
    DevExtTramp 4
    DevExtTramp 5
    DevExtTramp 6
    DevExtTramp 7
    DevExtTramp 8
    DevExtTramp 9
    DevExtTramp 10
    DevExtTramp 11
    DevExtTramp 12
    DevExtTramp 13
    DevExtTramp 14
    DevExtTramp 15
    DevExtTramp 16
    DevExtTramp 17
    DevExtTramp 18
    DevExtTramp 19
    DevExtTramp 20
    DevExtTramp 21
    DevExtTramp 22
    DevExtTramp 23
    DevExtTramp 24
    DevExtTramp 25
    DevExtTramp 26
    DevExtTramp 27
    DevExtTramp 28
    DevExtTramp 29
    DevExtTramp 30
    DevExtTramp 31
    DevExtTramp 32
    DevExtTramp 33
    DevExtTramp 34
    DevExtTramp 35
    DevExtTramp 36
    DevExtTramp 37
    DevExtTramp 38
    DevExtTramp 39
    DevExtTramp 40
    DevExtTramp 41
    DevExtTramp 42
    DevExtTramp 43
    DevExtTramp 44
    DevExtTramp 45
    DevExtTramp 46
    DevExtTramp 47
    DevExtTramp 48
    DevExtTramp 49
    DevExtTramp 50
    DevExtTramp 51
    DevExtTramp 52
    DevExtTramp 53
    DevExtTramp 54
    DevExtTramp 55
    DevExtTramp 56
    DevExtTramp 57
    DevExtTramp 58
    DevExtTramp 59
    DevExtTramp 60
    DevExtTramp 61
    DevExtTramp 62
    DevExtTramp 63
    DevExtTramp 64
    DevExtTramp 65
    DevExtTramp 66
    DevExtTramp 67
    DevExtTramp 68
    DevExtTramp 69
    DevExtTramp 70
    DevExtTramp 71
    DevExtTramp 72
    DevExtTramp 73
    DevExtTramp 74
    DevExtTramp 75
    DevExtTramp 76
    DevExtTramp 77
    DevExtTramp 78
    DevExtTramp 79
    DevExtTramp 80
    DevExtTramp 81
    DevExtTramp 82
    DevExtTramp 83
    DevExtTramp 84
    DevExtTramp 85
    DevExtTramp 86
    DevExtTramp 87
    DevExtTramp 88
    DevExtTramp 89
    DevExtTramp 90
    DevExtTramp 91
    DevExtTramp 92
    DevExtTramp 93
    DevExtTramp 94
    DevExtTramp 95
    DevExtTramp 96
    DevExtTramp 97
    DevExtTramp 98
    DevExtTramp 99
    DevExtTramp 100
    DevExtTramp 101
    DevExtTramp 102
    DevExtTramp 103
    DevExtTramp 104
    DevExtTramp 105
    DevExtTramp 106
    DevExtTramp 107
    DevExtTramp 108
    DevExtTramp 109
    DevExtTramp 110
    DevExtTramp 111
    DevExtTramp 112
    DevExtTramp 113
    DevExtTramp 114
    DevExtTramp 115
    DevExtTramp 116
    DevExtTramp 117
    DevExtTramp 118
    DevExtTramp 119
    DevExtTramp 120
    DevExtTramp 121
    DevExtTramp 122
    DevExtTramp 123
    DevExtTramp 124
    DevExtTramp 125
    DevExtTramp 126
    DevExtTramp 127
    DevExtTramp 128
    DevExtTramp 129
    DevExtTramp 130
    DevExtTramp 131
    DevExtTramp 132
    DevExtTramp 133
    DevExtTramp 134
    DevExtTramp 135
    DevExtTramp 136
    DevExtTramp 137
    DevExtTramp 138
    DevExtTramp 139
    DevExtTramp 140
    DevExtTramp 141
    DevExtTramp 142
    DevExtTramp 143
    DevExtTramp 144
    DevExtTramp 145
    DevExtTramp 146
    DevExtTramp 147
    DevExtTramp 148
    DevExtTramp 149
    DevExtTramp 150
    DevExtTramp 151
    DevExtTramp 152
    DevExtTramp 153
    DevExtTramp 154
    DevExtTramp 155
    DevExtTramp 156
    DevExtTramp 157
    DevExtTramp 158
    DevExtTramp 159
    DevExtTramp 160
    DevExtTramp 161
    DevExtTramp 162
    DevExtTramp 163
    DevExtTramp 164
    DevExtTramp 165
    DevExtTramp 166
    DevExtTramp 167
    DevExtTramp 168
    DevExtTramp 169
    DevExtTramp 170
    DevExtTramp 171
    DevExtTramp 172
    DevExtTramp 173
    DevExtTramp 174
    DevExtTramp 175
    DevExtTramp 176
    DevExtTramp 177
    DevExtTramp 178
    DevExtTramp 179
    DevExtTramp 180
    DevExtTramp 181
    DevExtTramp 182
    DevExtTramp 183
    DevExtTramp 184
    DevExtTramp 185
    DevExtTramp 186
    DevExtTramp 187
    DevExtTramp 188
    DevExtTramp 189
    DevExtTramp 190
    DevExtTramp 191
    DevExtTramp 192
    DevExtTramp 193
    DevExtTramp 194
    DevExtTramp 195
    DevExtTramp 196
    DevExtTramp 197
    DevExtTramp 198
    DevExtTramp 199
    DevExtTramp 200
    DevExtTramp 201
    DevExtTramp 202
    DevExtTramp 203
    DevExtTramp 204
    DevExtTramp 205
    DevExtTramp 206
    DevExtTramp 207
    DevExtTramp 208
    DevExtTramp 209
    DevExtTramp 210
    DevExtTramp 211
    DevExtTramp 212
    DevExtTramp 213
    DevExtTramp 214
    DevExtTramp 215
    DevExtTramp 216
    DevExtTramp 217
    DevExtTramp 218
    DevExtTramp 219
    DevExtTramp 220
    DevExtTramp 221
    DevExtTramp 222
    DevExtTramp 223
    DevExtTramp 224
    DevExtTramp 225
    DevExtTramp 226
    DevExtTramp 227
    DevExtTramp 228
    DevExtTramp 229
    DevExtTramp 230
    DevExtTramp 231
    DevExtTramp 232
    DevExtTramp 233
    DevExtTramp 234
    DevExtTramp 235
    DevExtTramp 236
    DevExtTramp 237
    DevExtTramp 238
    DevExtTramp 239
    DevExtTramp 240
    DevExtTramp 241
    DevExtTramp 242
    DevExtTramp 243
    DevExtTramp 244
    DevExtTramp 245
    DevExtTramp 246
    DevExtTramp 247
    DevExtTramp 248
    DevExtTramp 249
