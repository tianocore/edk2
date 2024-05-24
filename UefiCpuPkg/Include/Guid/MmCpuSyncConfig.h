/** @file
  This file defines MM_CPU_SYNC_CONFIG which controls how BSP synchronizes with APs
  in x86 SMM environment.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_CPU_SYNC_CONFIG_H_
#define MM_CPU_SYNC_CONFIG_H_

///
/// The GUID of the MmCpuSyncConfig GUIDed HOB.
///
#define MM_CPU_SYNC_CONFIG_HOB_GUID \
  { \
    0x8b90bd26, 0xe4f9, 0x45c2, {0x92, 0xa2, 0x9e, 0xac, 0xe6, 0x0e, 0x9d, 0xcc}  \
  }

typedef enum {
  MmCpuSyncModeTradition,
  MmCpuSyncModeRelaxedAp,
  MmCpuSyncModeMax
} MM_CPU_SYNC_MODE;

///
/// The structure defines the data layout of the MmCpuSyncConfig GUIDed HOB.
///
typedef struct {
  ///
  /// 0: Traditional CPU synchronization method is used when processing an SMI.
  /// 1: Relaxed CPU synchronization method is used when processing an SMI.
  ///
  MM_CPU_SYNC_MODE    RelaxedApMode;

  ///
  /// The 1st BSP/AP synchronization timeout value in SMM.
  /// The value shall match with the PcdCpuSmmApSyncTimeout.
  ///
  UINT64              Timeout;

  ///
  /// The 2nd BSP/AP synchronization timeout value in SMM.
  /// The value shall match with the PcdCpuSmmApSyncTimeout2.
  ///
  UINT64              Timeout2;
} MM_CPU_SYNC_CONFIG;

extern EFI_GUID  gMmCpuSyncConfigHobGuid;

#endif
