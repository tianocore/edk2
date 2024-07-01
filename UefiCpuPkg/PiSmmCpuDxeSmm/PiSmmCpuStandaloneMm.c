/** @file
Agent Module to load other modules to deploy MM Entry Vector for X86 CPU.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

//
// TRUE to indicate it's the MM_STANDALONE MM CPU driver.
// FALSE to indicate it's the DXE_SMM_DRIVER SMM CPU driver.
//
const BOOLEAN  mIsStandaloneMm = TRUE;

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
  EFI_HOB_GUID_TYPE    *GuidHob;
  SMM_CPU_SYNC_CONFIG  *SmmSyncModeInfoHob;

  SmmSyncModeInfoHob = NULL;

  //
  // Get SMM_CPU_SYNC_CONFIG_HOB for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiSmmCpuSyncConfigHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    SmmSyncModeInfoHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (SmmSyncModeInfoHob != NULL) {
    if (RelaxedMode != NULL) {
      *RelaxedMode = SmmSyncModeInfoHob->RelaxedMode;
    }

    if (SyncTimeout != NULL) {
      *SyncTimeout = SmmSyncModeInfoHob->SyncTimeout;
    }

    if (SyncTimeout2 != NULL) {
      *SyncTimeout2 = SmmSyncModeInfoHob->SyncTimeout2;
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
  ACPI_S3_ENABLE     *AcpiS3EnableHob;

  AcpiS3EnableHob = NULL;

  //
  // Get ACPI_S3_ENABLE_HOB for Standalone MM init.
  //
  GuidHob = GetFirstGuidHob (&gEdkiiAcpiS3EnableHobGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    AcpiS3EnableHob = GET_GUID_HOB_DATA (GuidHob);
  }

  if (AcpiS3EnableHob != NULL) {
    mAcpiS3Enable = AcpiS3EnableHob->AcpiS3Enable;
  }
}

/**
  Get the maximum number of logical processors supported by the system.

  @retval The maximum number of logical processors supported by the system
          is indicated by the return value.
**/
UINTN
GetSupportedMaxLogicalProcessorNumber (
  VOID
  )
{
  ASSERT (FALSE);

  return 0;
}

/**
  Extract NumberOfCpus, MaxNumberOfCpus and EFI_PROCESSOR_INFORMATION.

  @param[out] NumberOfCpus           Pointer to NumberOfCpus.
  @param[out] MaxNumberOfCpus        Pointer to MaxNumberOfCpus.

  @retval ProcessorInfo              Pointer to EFI_PROCESSOR_INFORMATION buffer.
**/
EFI_PROCESSOR_INFORMATION *
GetMpInformationFromMpServices (
  OUT UINTN  *NumberOfCpus,
  OUT UINTN  *MaxNumberOfCpus
  )
{
  ASSERT (FALSE);

  return NULL;
}
