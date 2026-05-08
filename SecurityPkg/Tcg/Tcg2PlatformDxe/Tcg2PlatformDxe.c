/** @file
  Platform specific TPM2 component for configuring the Platform Hierarchy.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/TpmPlatformHierarchyLib.h>
#include <Protocol/DxeSmmReadyToLock.h>

/**
   This callback function will run at EndOfDxe or ReadyToBoot based on boot mode.

   Configuration of the TPM's Platform Hierarchy Authorization Value (platformAuth)
   and Platform Hierarchy Authorization Policy (platformPolicy) can be defined through this function.

  @param  Event   Pointer to this event
  @param  Context Event handler private data
 **/
VOID
EFIAPI
TpmReadyToLockEventCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "[%a] Disabling TPM Platform Hierarchy\n", __func__));
  ConfigureTpmPlatformHierarchy ();

  gBS->CloseEvent (Event);
}

/**
   The driver's entry point. Will register a function for callback during ReadyToBoot event to
   configure the TPM's platform authorization.

   @param[in] ImageHandle  The firmware allocated handle for the EFI image.
   @param[in] SystemTable  A pointer to the EFI System Table.

   @retval EFI_SUCCESS     The entry point is executed successfully.
   @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
Tcg2PlatformDxeEntryPoint (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;
  EFI_EVENT      Event;

  BootMode = GetBootModeHob ();

  // In flash update boot path, leave TPM Platform Hierarchy enabled until ReadyToBoot (which should never actually
  // occur, since capsule reset will occur first).
  if (BootMode == BOOT_ON_FLASH_UPDATE) {
    Status = EfiCreateEventReadyToBootEx (TPL_CALLBACK, TpmReadyToLockEventCallBack, NULL, &Event);
  } else {
    // In all other boot paths, disable TPM Platform Hierarchy at EndOfDxe.
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    TpmReadyToLockEventCallBack,
                    NULL,
                    &gEfiEndOfDxeEventGroupGuid,
                    &Event
                    );
  }

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
