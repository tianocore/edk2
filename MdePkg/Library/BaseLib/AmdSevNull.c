/** @file

  Null stub of SevLib

  Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Uefi/UefiBaseType.h>

/**
  Probe if running as some kind of SEV guest.

  @return FALSE   Not running as a guest under any kind of SEV
  @return TRUE    Running as a guest under any kind of SEV
**/
BOOLEAN
EFIAPI
SevGuestIsEnabled (
  VOID
  )
{
  return FALSE;
}
