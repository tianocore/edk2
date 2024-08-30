/** @file
  LockBox MM driver.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/StandaloneMmDriverEntryPoint.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/StandaloneMmMemLib.h>
#include <Library/LockBoxLib.h>

#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/LockBox.h>
#include <Guid/SmmLockBox.h>

#include "SmmLockBoxCommon.h"

/**
  This function is an abstraction layer for implementation specific Mm buffer validation routine.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with SMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with SMRAM.
**/
BOOLEAN
IsBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return TRUE;
}

/**
  Entry Point for LockBox MM driver.
  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.
  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
SmmLockBoxStandaloneMmEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return SmmLockBoxEntryPointCommon ();
}
