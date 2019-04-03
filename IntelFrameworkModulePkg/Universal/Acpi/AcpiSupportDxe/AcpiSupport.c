/** @file
  This is an implementation of the ACPI Support protocol.  This is defined in
  the Tiano ACPI External Product Specification, revision 0.3.6.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Includes
//
#include "AcpiSupport.h"

//
// Handle to install ACPI Table Protocol (and ACPI Suppport protocol).
//
EFI_HANDLE    mHandle = NULL;

/**
  Entry point of the ACPI support driver. This function creates and initializes an instance of the ACPI Support
  Protocol and installs it on a new handle.

  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table

  @retval EFI_SUCCESS              Driver initialized successfully
  @retval EFI_LOAD_ERROR           Failed to Initialize or has been loaded
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources
**/
EFI_STATUS
EFIAPI
InstallAcpiSupport (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )

{
  EFI_STATUS                Status;
  EFI_ACPI_SUPPORT_INSTANCE *PrivateData;

  //
  // Initialize our protocol
  //
  PrivateData = AllocateZeroPool (sizeof (EFI_ACPI_SUPPORT_INSTANCE));
  ASSERT (PrivateData);
  PrivateData->Signature = EFI_ACPI_SUPPORT_SIGNATURE;

  //
  // Call all constructors per produced protocols
  //
  Status = AcpiSupportAcpiSupportConstructor (PrivateData);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (PrivateData);
    return EFI_LOAD_ERROR;
  }

  //
  // Install ACPI Table protocol and optional ACPI support protocol based on
  // feature flag: PcdInstallAcpiSupportProtocol.
  //
  if (FeaturePcdGet (PcdInstallAcpiSupportProtocol)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mHandle,
                    &gEfiAcpiTableProtocolGuid,
                    &PrivateData->AcpiTableProtocol,
                    &gEfiAcpiSupportProtocolGuid,
                    &PrivateData->AcpiSupport,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mHandle,
                    &gEfiAcpiTableProtocolGuid,
                    &PrivateData->AcpiTableProtocol,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
