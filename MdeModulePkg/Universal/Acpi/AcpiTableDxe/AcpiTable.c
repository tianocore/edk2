/** @file
  ACPI Table Protocol Driver

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Includes
//
#include "AcpiTable.h"

//
// Handle to install ACPI Table Protocol
//
EFI_HANDLE                                             mHandle       = NULL;
GLOBAL_REMOVE_IF_UNREFERENCED EFI_ACPI_TABLE_INSTANCE  *mPrivateData = NULL;

/**
  Entry point of the ACPI table driver.
  Creates and initializes an instance of the ACPI Table
  Protocol and installs it on a new handle.

  @param  ImageHandle   A handle for the image that is initializing this driver.
  @param  SystemTable   A pointer to the EFI system table.

  @return EFI_SUCCESS           Driver initialized successfully.
  @return EFI_LOAD_ERROR        Failed to Initialize or has been loaded.
  @return EFI_OUT_OF_RESOURCES  Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
InitializeAcpiTableDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_INSTANCE  *PrivateData;

  //
  // Initialize ACPI Table instance
  //
  PrivateData = AllocateZeroPool (sizeof (EFI_ACPI_TABLE_INSTANCE));
  if (PrivateData != NULL) {
    PrivateData->Signature = EFI_ACPI_TABLE_SIGNATURE;
  } else {
    ASSERT (PrivateData);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Call all constructors per produced protocols
  //
  Status = AcpiTableAcpiTableConstructor (PrivateData);
  if (EFI_ERROR (Status)) {
    gBS->FreePool (PrivateData);
    return EFI_LOAD_ERROR;
  }

  mPrivateData = PrivateData;
  //
  // Install ACPI Table protocol
  //
  if (FeaturePcdGet (PcdInstallAcpiSdtProtocol)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mHandle,
                    &gEfiAcpiTableProtocolGuid,
                    &mPrivateData->AcpiTableProtocol,
                    &gEfiAcpiSdtProtocolGuid,
                    &mPrivateData->AcpiSdtProtocol,
                    NULL
                    );
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mHandle,
                    &gEfiAcpiTableProtocolGuid,
                    &mPrivateData->AcpiTableProtocol,
                    NULL
                    );
  }

  ASSERT_EFI_ERROR (Status);

  return Status;
}
