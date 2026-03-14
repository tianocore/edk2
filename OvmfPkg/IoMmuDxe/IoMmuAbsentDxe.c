/** @file

  IoMmuDxe driver that installs gIoMmuAbsentProtocolGuid for
  guest VMs for which confidential computing support has not
  yet been enabled.

  Copyright (c) 2017, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
IoMmuDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;

  //
  // For Guest VMs for which CC is not yet enabled
  // install the gIoMmuAbsentProtocolGuid for dependent
  // modules to continue to load.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gIoMmuAbsentProtocolGuid,
                  NULL,
                  NULL
                  );
  return Status;
}
