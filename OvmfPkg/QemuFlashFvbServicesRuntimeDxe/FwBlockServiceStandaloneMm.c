/**@file
  Functions related to the Firmware Volume Block service whose
  implementation is specific to the SMM driver build.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>

#include "FwBlockService.h"

VOID
InstallProtocolInterfaces (
  IN EFI_FW_VOL_BLOCK_DEVICE  *FvbDevice
  )
{
  EFI_HANDLE  FvbHandle;
  EFI_STATUS  Status;

  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  //
  // There is no SMM service that can install multiple protocols in the SMM
  // protocol database in one go.
  //
  // The SMM Firmware Volume Block protocol structure is the same as the
  // Firmware Volume Block protocol structure.
  //
  FvbHandle = NULL;
  DEBUG ((DEBUG_INFO, "Installing QEMU flash SMM FVB\n"));
  Status = gMmst->MmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &FvbDevice->FwVolBlockInstance
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gMmst->MmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    FvbDevice->DevicePath
                    );
  ASSERT_EFI_ERROR (Status);
}

VOID
InstallVirtualAddressChangeHandler (
  VOID
  )
{
  //
  // Nothing.
  //
}

EFI_STATUS
MarkIoMemoryRangeForRuntimeAccess (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length
  )
{
  //
  // Nothing
  //

  return EFI_SUCCESS;
}

VOID
SetPcdFlashNvStorageBaseAddresses (
  VOID
  )
{
  //
  // Do nothing.
  //
}

VOID
UpdateQemuFlashVariablesEnable (
  VOID
  )
{
  //
  // Do nothing for Standalone MM.
  //
}

/**
  Abstracted entry point for Standalone MM instance.
  FVB Standalone MM driver entry point.

  @param[in] ImageHandle    A handle for the image that is initializing this
                            driver
  @param[in] MmSystemTable  A pointer to the MM system table

  @retval EFI_SUCCESS       Variable service successfully initialized.
**/
EFI_STATUS
EFIAPI
FvbInitializeStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return FvbInitialize (NULL, NULL);
}
