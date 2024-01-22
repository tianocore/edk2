/** @file
  This PEI Module creates SMM_CPU_FEATURE_INFO_HOB.

  Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmCpuFeatureInfoPeim.h"

/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
SmmCpuFeatureInfoPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  SMM_CPU_FEATURE_INFO_HOB  *SmmSyncModeInfoHob;

  SmmSyncModeInfoHob = BuildGuidHob (&gSmmCpuFeatureInfoGuid, sizeof (SMM_CPU_FEATURE_INFO_HOB));
  ASSERT (SmmSyncModeInfoHob != NULL);

  SmmSyncModeInfoHob->RelaxedCpuSyncMode  = (BOOLEAN)PcdGet8 (PcdCpuSmmSyncMode);
  SmmSyncModeInfoHob->AcpiS3Enable        = (BOOLEAN)PcdGetBool (PcdAcpiS3Enable);
  SmmSyncModeInfoHob->CpuSmmApSyncTimeout = PcdGet64 (PcdCpuSmmApSyncTimeout);

  return EFI_SUCCESS;
}
