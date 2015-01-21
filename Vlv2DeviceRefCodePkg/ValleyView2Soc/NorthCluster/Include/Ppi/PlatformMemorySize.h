/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  PlatformMemorySize.h

Abstract:

  Platform Memory Size PPI as defined in Tiano

  PPI for describing the minimum platform memory size in order to successfully
  pass control into DXE

--*/
//
//
#ifndef _PEI_PLATFORM_MEMORY_SIZE_H_
#define _PEI_PLATFORM_MEMORY_SIZE_H_

#define PEI_PLATFORM_MEMORY_SIZE_PPI_GUID \
  { \
    0x9a7ef41e, 0xc140, 0x4bd1, 0xb8, 0x84, 0x1e, 0x11, 0x24, 0xb, 0x4c, 0xe6 \
  }

EFI_FORWARD_DECLARATION (PEI_PLATFORM_MEMORY_SIZE_PPI);

typedef
EFI_STATUS
(EFIAPI *PEI_GET_MINIMUM_PLATFORM_MEMORY_SIZE) (
  IN      EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_PLATFORM_MEMORY_SIZE_PPI                * This,
  IN OUT  UINT64                                 *MemorySize
  );

typedef struct _PEI_PLATFORM_MEMORY_SIZE_PPI {
  PEI_GET_MINIMUM_PLATFORM_MEMORY_SIZE  GetPlatformMemorySize;
} PEI_PLATFORM_MEMORY_SIZE_PPI;

extern EFI_GUID gPeiPlatformMemorySizePpiGuid;

#endif
