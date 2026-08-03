/** @file
  BootNext preservation policy for BDS.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BDS_BOOT_NEXT_POLICY_H_
#define BDS_BOOT_NEXT_POLICY_H_

#include <Uefi.h>
#include <Pi/PiBootMode.h>

BOOLEAN
BdsShouldPreserveCapsuleBootNext (
  IN BOOLEAN        BootNextPresent,
  IN BOOLEAN        CapsuleDeliveryRequested,
  IN EFI_BOOT_MODE  BootMode
  );

#endif
