/** @file
  LockBox Dependency DXE Library.

  By installing the LockBox protocol with the gEfiLockBoxProtocolGuid,
  it signals that the LockBox API is fully operational and ready for use.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/LockBox.h>

/**
  The constructor function of SmmLockBoxMmDependency.

  It attempts to install the gEfiLockBoxProtocolGuid protocol into the system's DXE database
  with NULL as notify.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the Management mode System Table.

  @retval EFI_SUCCESS           The protocol was successfully installed into the DXE database.
  @retval Others                An error occurred while installing the protocol.
**/
EFI_STATUS
EFIAPI
SmmLockBoxMmDependencyConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install NULL to DXE data base as notify
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiLockBoxProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
