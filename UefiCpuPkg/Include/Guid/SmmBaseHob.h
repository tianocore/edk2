/** @file
  The Smm Base HOB is used to store the information of:
  * The relocated SmBase in array for each Processors.

  The default SMBASE for the x86 processor is 0x30000. When SMI happens,
  CPU runs the SMI handler at SMBASE+0x8000. Also, the SMM save state
  area is within SMBASE+0x10000.

  One of the SMM initialization from CPU perspective is to program the
  new SMBASE (in TSEG range) for each CPU thread. When the SMBASE update
  happens in a PEI module, the PEI module shall produce the SMM_BASE_HOB
  in HOB database which tells the PiSmmCpuDxeSmm driver which runs at a
  later phase about the new SMBASE for each CPU thread. PiSmmCpuDxeSmm
  driver installs the SMI handler at the SMM_BASE_HOB.SmBase[Index]+0x8000
  for CPU thread Index. When the HOB doesn't exist, PiSmmCpuDxeSmm driver
  shall program the new SMBASE itself.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_BASE_HOB_H_
#define SMM_BASE_HOB_H_

#include <Protocol/MpService.h>
#include <PiPei.h>

#define SMM_BASE_HOB_DATA_GUID \
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
  /// Pointer to SmBase address for each Processors.
  ///
  UINT64    SmBase[];
} SMM_BASE_HOB_DATA;
#pragma pack()

extern EFI_GUID  gSmmBaseHobGuid;

#endif
