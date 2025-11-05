/**
  NULL implementation for UnitTestBootLib to allow simple compilation

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>

/**
  Set the boot manager to boot from a specific device on the next boot. This
  should be set only for the next boot and shouldn't require any manual clean up

  @retval EFI_SUCCESS      Boot device for next boot was set.
  @retval EFI_UNSUPPORTED  Setting the boot device for the next boot is not
                           supportted.
  @retval Other            Boot device for next boot can not be set.
**/
EFI_STATUS
EFIAPI
SetBootNextDevice (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
