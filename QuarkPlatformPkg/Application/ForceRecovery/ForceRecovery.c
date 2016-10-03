/** @file
  Application that sets a sticky bit to force recovery on next reset.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
