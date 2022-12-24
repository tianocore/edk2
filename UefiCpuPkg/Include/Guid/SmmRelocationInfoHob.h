/** @file
  The Smm Relocation Info HOB is used to store the information of:
    A. Smm Relocated SmBase array for each Processors;

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_RELOCATION_INFO_HOB_H_
#define SMM_RELOCATION_INFO_HOB_H_

#include <Protocol/MpService.h>
#include <PiPei.h>

#define SMM_RELOCATION_INFO_HOB_DATA_GUID \
  { \
    0xc2217ba7, 0x03bb, 0x4f63, {0xa6, 0x47, 0x7c, 0x25, 0xc5, 0xfc, 0x9d, 0x73}  \
  }

#pragma pack(1)
typedef struct {
  ///
  /// Describes the Number of all max supported processors.
  ///
  UINT64    NumberOfProcessors;
  ///
  /// Pointer to SmBase array for each Processors.
  ///
  UINT64    SmBase[];
} SMM_RELOCATION_INFO_HOB_DATA;
#pragma pack()

extern EFI_GUID  gSmmRelocationInfoHobGuid;

#endif
