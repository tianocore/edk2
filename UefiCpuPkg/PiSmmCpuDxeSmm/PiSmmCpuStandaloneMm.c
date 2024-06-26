/** @file
Agent Module to load other modules to deploy MM Entry Vector for X86 CPU.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  To get system port address of the SMI Command Port.

**/
VOID
GetSmiCommandPort (
  VOID
  )
{
  mSmiCommandPort = 0xB2;
  DEBUG ((DEBUG_INFO, "mSmiCommandPort = %x\n", mSmiCommandPort));
}

/**
  Get SmmCpuSyncConfig data: RelaxedMode, SyncTimeout, SyncTimeout2.

  @param[in,out] RelaxedMode   It indicates if Relaxed CPU synchronization method or
                               traditional CPU synchronization method is used when processing an SMI.
  @param[in,out] SyncTimeout   It indicates the 1st BSP/AP synchronization timeout value in SMM.
  @param[in,out] SyncTimeout2  It indicates the 2nd BSP/AP synchronization timeout value in SMM.

 **/
VOID
GetSmmCpuSyncConfigData (
  IN OUT BOOLEAN *RelaxedMode, OPTIONAL
  IN OUT UINT64  *SyncTimeout, OPTIONAL
  IN OUT UINT64  *SyncTimeout2 OPTIONAL
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  MM_CPU_SYNC_CONFIG  *MmCpuSyncConfigHob;

  MmCpuSyncConfigHob = NULL;

  //
  // Get MM_CPU_SYNC_CONFIG for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gMmCpuSyncConfigHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    MmCpuSyncConfigHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (MmCpuSyncConfigHob != NULL) {
    if (RelaxedMode != NULL) {
      *RelaxedMode = ((MmCpuSyncConfigHob->RelaxedApMode == MmCpuSyncModeRelaxedAp) ? TRUE : FALSE);
    }

    if (SyncTimeout != NULL) {
      *SyncTimeout = MmCpuSyncConfigHob->Timeout;
    }

    if (SyncTimeout2 != NULL) {
      *SyncTimeout2 = MmCpuSyncConfigHob->Timeout2;
    }
  }
}

/**
  Get ACPI S3 enable flag.

**/
VOID
GetAcpiS3EnableFlag (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  MM_ACPI_S3_ENABLE  *MmAcpiS3EnableHob;

  MmAcpiS3EnableHob = NULL;

  //
  // Get MM_ACPI_S3_ENABLE for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gMmAcpiS3EnableHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    MmAcpiS3EnableHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (MmAcpiS3EnableHob != NULL) {
    mAcpiS3Enable = MmAcpiS3EnableHob->AcpiS3Enable;
  }
}
