/** @file
  MM Core FV location PPI header file.

  MM Core FV location PPI is used by StandaloneMm IPL to find MM Core.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_CORE_FV_LOCATION_PPI_H_
#define MM_CORE_FV_LOCATION_PPI_H_

#pragma pack(1)

///
/// Global ID for the MM_CORE_FV_LOCATION_PPI.
///
#define MM_CORE_FV_LOCATION_GUID \
  { \
    0x47a00618, 0x237a, 0x4386, { 0x8f, 0xc5, 0x2a, 0x86, 0xd8, 0xac, 0x41, 0x05 } \
  }

typedef struct {
  EFI_PHYSICAL_ADDRESS    Address;
  UINT64                  Size;
} MM_CORE_FV_LOCATION_PPI;

extern EFI_GUID  gMmCoreFvLocationPpiGuid;

#pragma pack()

#endif
