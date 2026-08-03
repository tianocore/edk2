/** @file
  BootNext preservation policy for BDS.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootNextPolicy.h"

BOOLEAN
BdsShouldPreserveCapsuleBootNext (
  IN BOOLEAN        BootNextPresent,
  IN BOOLEAN        CapsuleDeliveryRequested,
  IN EFI_BOOT_MODE  BootMode
  )
{
  return (BOOLEAN)(
                    BootNextPresent &&
                    CapsuleDeliveryRequested &&
                    (BootMode == BOOT_ON_S4_RESUME)
                    );
}
