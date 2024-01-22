/** @file
  This file defines a SMM_CPU_FEATURE_INFO_HOB.
  It indicates if Relaxed CPU synchronization method or traditional
  CPU synchronization method is used when processing an SMI.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CPU_FEATURE_INFO_H_
#define SMM_CPU_FEATURE_INFO_H_

#include <PiPei.h>

#define SMM_SYNC_MODE_INFO_HOB_REVISION  1

#define SMM_CPU_FEATURE_INFO_GUID \
  { \
    0x8b90bd26, 0xe4f9, 0x45c2, {0x92, 0xa2, 0x9e, 0xac, 0xe6, 0x0e, 0x9d, 0xcc}  \
  }

#pragma pack(1)
typedef struct {
  BOOLEAN    RelaxedCpuSyncMode;
  BOOLEAN    AcpiS3Enable;
  UINT64     CpuSmmApSyncTimeout;
} SMM_CPU_FEATURE_INFO_HOB;
#pragma pack()

extern EFI_GUID  gSmmCpuFeatureInfoGuid;

#endif
