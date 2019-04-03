/** @file
  Application that sets a sticky bit to force recovery on next reset.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/QNCAccessLib.h>

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Set 'B_CFG_STICKY_RW_FORCE_RECOVERY' sticky bit so we know we need to do a recovery following warm reset
  //
  QNCAltPortWrite (
    QUARK_SCSS_SOC_UNIT_SB_PORT_ID,
    QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW,
    QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW) | B_CFG_STICKY_RW_FORCE_RECOVERY
    );

  //
  // Do a warm reset
  //
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);

  return EFI_SUCCESS;
}
