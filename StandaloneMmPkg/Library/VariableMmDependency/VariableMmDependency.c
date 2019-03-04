/** @file
  Runtime DXE part corresponding to StanaloneMM variable module.

This module installs variable arch protocol and variable write arch protocol
to StandaloneMM runtime variable service.

Copyright (c) 2019, ARM Ltd. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  The constructor function installs variable arch protocol and variable
  write arch protocol to StandaloneMM runtime variable service

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the Management mode System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
VariableMmDependencyLibConstructor (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiSmmVariableProtocolGuid,
                  NULL,
                  &gSmmVariableWriteGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}
