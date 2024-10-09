/** @file
  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/FspCommonLib.h>
#include <FspEas.h>
#include <Library/FspSwitchStackLib.h>

/**
  This function updates the return status of the FSP API with requested reset type and returns to Boot Loader.

  @param[in] FspResetType     Reset type that needs to returned as API return status

**/
VOID
EFIAPI
FspApiReturnStatusReset (
  IN EFI_STATUS  FspResetType
  )
{
  volatile BOOLEAN  LoopUntilReset;

  LoopUntilReset = TRUE;
  DEBUG ((DEBUG_INFO, "FSP returning control to Bootloader with reset required return status %x\n", FspResetType));
  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    ///
    /// Below code is not an infinite loop.The control will go back to API calling function in BootLoader each time BootLoader
    /// calls the FSP API without honoring the reset request by FSP
    ///
    do {
      SetFspApiReturnStatus (FspResetType);
      Pei2LoaderSwitchStack ();
      DEBUG ((DEBUG_ERROR, "!!!ERROR: FSP has requested BootLoader for reset. But BootLoader has not honored the reset\n"));
      DEBUG ((DEBUG_ERROR, "!!!ERROR: Please add support in BootLoader to honor the reset request from FSP\n"));
    } while (LoopUntilReset);
  }
}
