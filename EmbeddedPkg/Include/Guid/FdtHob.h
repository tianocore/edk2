/** @file
  GUID for the HOB that contains the copy of the flattened device tree blob

  Copyright (C) 2014, Linaro Ltd.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FDT_HOB_H__
#define __FDT_HOB_H__

#define FDT_HOB_GUID { \
          0x16958446, 0x19B7, 0x480B, \
          { 0xB0, 0x47, 0x74, 0x85, 0xAD, 0x3F, 0x71, 0x6D } \
        }

extern EFI_GUID gFdtHobGuid;

#endif
