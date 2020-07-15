/** @file
  EFI PEI Core Security services

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"


EFI_PEI_NOTIFY_DESCRIPTOR mNotifyList = {
   EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
   &gEfiPeiSecurity2PpiGuid,
   SecurityPpiNotifyCallback
};

/**
  Initialize the security services.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param OldCoreData     Pointer to the old core data.
                         NULL if being run in non-permanent memory mode.

**/
VOID
InitializeSecurityServices (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
{
  if (OldCoreData == NULL) {
    PeiServicesNotifyPpi (&mNotifyList);
  }
  return;
}

/**

  Provide a callback for when the security PPI is installed.
  This routine will cache installed security PPI into PeiCore's private data.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyDescriptor   The descriptor for the notification event.
  @param Ppi                Pointer to the PPI in question.

  @return Always success

**/
EFI_STATUS
EFIAPI
SecurityPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_CORE_INSTANCE                       *PrivateData;

  //
  // Get PEI Core private data
  //
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  //
  // If there isn't a security PPI installed, use the one from notification
  //
  if (PrivateData->PrivateSecurityPpi == NULL) {
    PrivateData->PrivateSecurityPpi = (EFI_PEI_SECURITY2_PPI *)Ppi;
  }
  return EFI_SUCCESS;
}

/**
  Provide a callout to the security verification service.

  @param PrivateData     PeiCore's private data structure
  @param VolumeHandle    Handle of FV
  @param FileHandle      Handle of PEIM's FFS
  @param AuthenticationStatus Authentication status

  @retval EFI_SUCCESS              Image is OK
  @retval EFI_SECURITY_VIOLATION   Image is illegal
  @retval EFI_NOT_FOUND            If security PPI is not installed.
**/
EFI_STATUS
VerifyPeim (
  IN PEI_CORE_INSTANCE      *PrivateData,
  IN EFI_PEI_FV_HANDLE      VolumeHandle,
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN UINT32                 AuthenticationStatus
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         DeferExecution;

  Status = EFI_NOT_FOUND;
  if (PrivateData->PrivateSecurityPpi == NULL) {
    //
    // Check AuthenticationStatus first.
    //
    if ((AuthenticationStatus & EFI_AUTH_STATUS_IMAGE_SIGNED) != 0) {
      if ((AuthenticationStatus & (EFI_AUTH_STATUS_TEST_FAILED | EFI_AUTH_STATUS_NOT_TESTED)) != 0) {
        Status = EFI_SECURITY_VIOLATION;
      }
    }
  } else {
    //
    // Check to see if the image is OK
    //
    Status = PrivateData->PrivateSecurityPpi->AuthenticationState (
                                                (CONST EFI_PEI_SERVICES **) &PrivateData->Ps,
                                                PrivateData->PrivateSecurityPpi,
                                                AuthenticationStatus,
                                                VolumeHandle,
                                                FileHandle,
                                                &DeferExecution
                                                );
    if (DeferExecution) {
      Status = EFI_SECURITY_VIOLATION;
    }
  }
  return Status;
}


/**
  Verify a Firmware volume.

  @param CurrentFvAddress   Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS       Firmware Volume is legal

**/
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
{
  //
  // Right now just pass the test.  Future can authenticate and/or check the
  // FV-header or other metric for goodness of binary.
  //
  return EFI_SUCCESS;
}
