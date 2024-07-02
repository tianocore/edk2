/** @file
  This file defines a SMM_CPU_SYNC_CONFIG.

  SMM_CPU_SYNC_CONFIG structure including following parameters:
  1.RelaxedMode: It indicates if Relaxed CPU synchronization method or
  traditional CPU synchronization method is used when processing an SMI. The value
  shall match with the PcdCpuSmmSyncMode.
  2.SyncTimeout: It indicates Timeout value during SMM CPU Sync. The value
  shall match with the PcdCpuSmmApSyncTimeout.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_CPU_SYNC_CONFIG_H_
#define SMM_CPU_SYNC_CONFIG_H_

#include <PiPei.h>

#define SMM_CPU_SYNC_CONFIG_GUID \
  { \
    0x8b90bd26, 0xe4f9, 0x45c2, {0x92, 0xa2, 0x9e, 0xac, 0xe6, 0x0e, 0x9d, 0xcc}  \
  }

#pragma pack(1)
typedef struct {
  BOOLEAN    RelaxedMode;
  UINT64     SyncTimeout;
} SMM_CPU_SYNC_CONFIG;
#pragma pack()

extern EFI_GUID  gEdkiiSmmCpuSyncConfigHobGuid;

#endif
