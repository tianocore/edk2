/** @file
  The driver entry point for RamDiskDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RamDiskImpl.h"

//
// Handle for the EFI_RAM_DISK_PROTOCOL instance
//
EFI_HANDLE  mRamDiskHandle = NULL;

//
// The EFI_RAM_DISK_PROTOCOL instances that is installed onto the driver
// handle
//
EFI_RAM_DISK_PROTOCOL  mRamDiskProtocol = {
  RamDiskRegister,
  RamDiskUnregister
};

//
// RamDiskDxe driver maintains a list of registered RAM disks.
//
LIST_ENTRY  RegisteredRamDisks;
UINTN       ListEntryNum;


/**
  The entry point for RamDiskDxe driver.

  @param[in] ImageHandle     The image handle of the driver.
  @param[in] SystemTable     The system table.

  @retval EFI_ALREADY_STARTED     The driver already exists in system.
  @retval EFI_OUT_OF_RESOURCES    Fail to execute entry point due to lack of
                                  resources.
  @retval EFI_SUCCES              All the related protocols are installed on
                                  the driver.

**/
EFI_STATUS
EFIAPI
RamDiskDxeEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  RAM_DISK_CONFIG_PRIVATE_DATA    *ConfigPrivate;
  VOID                            *DummyInterface;

  //
  // If already started, return.
  //
  Status = gBS->LocateProtocol (
                  &gEfiRamDiskProtocolGuid,
                  NULL,
                  &DummyInterface
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Driver already started!\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Create a private data structure.
  //
  ConfigPrivate = AllocateCopyPool (sizeof (RAM_DISK_CONFIG_PRIVATE_DATA), &mRamDiskConfigPrivateDataTemplate);
  if (ConfigPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Install RAM disk configuration form
  //
  Status = InstallRamDiskConfigForm (ConfigPrivate);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install the EFI_RAM_DISK_PROTOCOL and RAM disk private data onto a
  // new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mRamDiskHandle,
                  &gEfiRamDiskProtocolGuid,
                  &mRamDiskProtocol,
                  &gEfiCallerIdGuid,
                  ConfigPrivate,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Initialize the list of registered RAM disks maintained by the driver
  //
  InitializeListHead (&RegisteredRamDisks);

  return EFI_SUCCESS;

ErrorExit:
  if (ConfigPrivate != NULL) {
    UninstallRamDiskConfigForm (ConfigPrivate);
  }

  return Status;
}


/**
  Unload the RamDiskDxe driver and its configuration form.

  @param[in] ImageHandle     The driver's image handle.

  @retval EFI_SUCCESS             The RamDiskDxe driver and its configuration
                                  form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
EFIAPI
RamDiskDxeUnload (
  IN EFI_HANDLE                   ImageHandle
  )
{
  EFI_STATUS                      Status;
  RAM_DISK_CONFIG_PRIVATE_DATA    *ConfigPrivate;

  Status = gBS->HandleProtocol (
                  mRamDiskHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &ConfigPrivate
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (ConfigPrivate->Signature == RAM_DISK_CONFIG_PRIVATE_DATA_SIGNATURE);

  //
  // Unregister all registered RAM disks
  //
  UnregisterAllRamDisks ();

  gBS->UninstallMultipleProtocolInterfaces (
         mRamDiskHandle,
         &gEfiRamDiskProtocolGuid,
         &mRamDiskProtocol,
         &gEfiCallerIdGuid,
         ConfigPrivate,
         NULL
         );

  UninstallRamDiskConfigForm (ConfigPrivate);

  return EFI_SUCCESS;
}
