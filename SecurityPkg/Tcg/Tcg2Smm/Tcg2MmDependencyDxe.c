/** @file
  Runtime DXE part corresponding to StandaloneMM Tcg2 module.

This module installs gTcg2MmSwSmiRegisteredGuid to notify readiness of
StandaloneMM Tcg2 module.

Copyright (c) 2019 - 2021, Arm Ltd. All rights reserved.
Copyright (c) Microsoft Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  The constructor function installs gTcg2MmSwSmiRegisteredGuid to notify
  readiness of StandaloneMM Tcg2 module.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the Management mode System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
Tcg2MmDependencyDxeEntryPoint (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            Handle;

  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gTcg2MmSwSmiRegisteredGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}
