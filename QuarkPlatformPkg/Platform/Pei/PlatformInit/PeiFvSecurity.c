/** @file
EFI PEI Platform Security services

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiFvSecurity.h"

EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnFvInfoSecurityList = {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolmeInfoPpiNotifySecurityCallback
};

/**
  Callback function to perform FV security checking on a FV Info PPI.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifySecurityCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  EFI_STATUS  Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI      *FvInfoPpi;
  EFI_PEI_FIRMWARE_VOLUME_PPI           *FvPpi;

  FvInfoPpi = (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *)Ppi;

  //
  // Locate the corresponding FV_PPI according to founded FV's format guid
  //
  Status = PeiServicesLocatePpi (
             &FvInfoPpi->FvFormat,
             0,
             NULL,
             (VOID**)&FvPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Only authenticate parent Firmware Volume (child firmware volumes are covered by the parent)
  //
  if ((VOID *)FvInfoPpi->ParentFvName == NULL && (VOID *)FvInfoPpi->ParentFileName == NULL) {
    Status = PeiSecurityVerifyFv ((EFI_FIRMWARE_VOLUME_HEADER*) FvInfoPpi->FvInfo);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Authenticates the Firmware Volume

  @param CurrentFvAddress   Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS       Firmware Volume is legal

**/
EFI_STATUS
PeiSecurityVerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
{
  EFI_STATUS  Status;

  //
  // Call Security library to authenticate the Firmware Volume
  //
  DEBUG ((DEBUG_INFO, "PeiSecurityVerifyFv - CurrentFvAddress=0x%8x\n", (UINT32)CurrentFvAddress));
  Status = EFI_SUCCESS;

  return Status;
}

/**

  Entry point for the PEI Security PEIM
  Sets up a notification to perform PEI security checking

  @param  FfsHeader    Not used.
  @param  PeiServices  General purpose services available to every PEIM.

  @return EFI_SUCCESS  PEI Security notification installed successfully.
          All others: PEI Security notification failed to install.

**/
EFI_STATUS
PeiInitializeFvSecurity (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesNotifyPpi (&mNotifyOnFvInfoSecurityList);

  return Status;
}

