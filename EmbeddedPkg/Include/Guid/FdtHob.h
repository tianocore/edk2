/** @file
  GUID for the HOB that contains the copy of the flattened device tree blob

  Copyright (C) 2014, Linaro Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FDT_HOB_H__
#define __FDT_HOB_H__

#define FDT_HOB_GUID { \
          0x16958446, 0x19B7, 0x480B, \
          { 0xB0, 0x47, 0x74, 0x85, 0xAD, 0x3F, 0x71, 0x6D } \
        }

extern EFI_GUID gFdtHobGuid;

#endif
