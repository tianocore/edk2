/** @file
  EFI application to test Arm CCA Boot Sync.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

/**
  Entrypoint for the Boot Sync Test application.

  @param[in] ImageHandle  Handle to the image
  @param[in] SystemTable  Pointer to System Table.

  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print (L"Boot Sync Test Application\n");

  return EFI_SUCCESS;
}
