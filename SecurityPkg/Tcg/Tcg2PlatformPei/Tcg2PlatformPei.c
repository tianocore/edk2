/** @file
  Configure TPM 2 platform hierarchy on TPM state resume failure on S3 resume

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/TpmPlatformHierarchyLib.h>
#include <Library/RngLib.h>

#include <Ppi/EndOfPeiPhase.h>

#define MAX_NEW_AUTHORIZATION_SIZE  SHA512_DIGEST_SIZE

/**
  This function handles PlatformInit task at the end of PEI

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS        The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PlatformInitEndOfPei (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  VOID  *TcgEventLog;

  //
  // Try to get TcgEventLog in S3 to see if S3 error is reported.
  //
  TcgEventLog = GetFirstGuidHob (&gTcgEventEntryHobGuid);
  if (TcgEventLog == NULL) {
    TcgEventLog = GetFirstGuidHob (&gTcgEvent2EntryHobGuid);
  }

  if (TcgEventLog == NULL) {
    //
    // no S3 error reported
    //
    return EFI_SUCCESS;
  }

  //
  // If there is S3 error on TPM_SU_STATE and success on TPM_SU_CLEAR,
  // configure the TPM Platform Hierarchy.
  //
  ConfigureTpmPlatformHierarchy ();

  return EFI_SUCCESS;
}

static EFI_PEI_NOTIFY_DESCRIPTOR  mEndOfPeiNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT)PlatformInitEndOfPei
};

/**
  Main entry

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

  @retval EFI_SUCCESS Install function successfully.

**/
EFI_STATUS
EFIAPI
Tcg2PlatformPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode != BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  //
  // Performing PlatformInitEndOfPei after EndOfPei PPI produced
  //
  Status = PeiServicesNotifyPpi (&mEndOfPeiNotifyList);

  return Status;
}
