/** @file
Define a GUID name for GUID HOB which is used to pass Memory
Configuration Data information to different modules.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_CONFIG_DATA_H_
#define _MEMORY_CONFIG_DATA_H_

#define EFI_MEMORY_CONFIG_DATA_GUID \
  { \
    0x80dbd530, 0xb74c, 0x4f11, {0x8c, 0x03, 0x41, 0x86, 0x65, 0x53, 0x28, 0x31 } \
  }

#define EFI_MEMORY_CONFIG_DATA_NAME  L"MemoryConfig"

extern EFI_GUID gEfiMemoryConfigDataGuid;

#endif
