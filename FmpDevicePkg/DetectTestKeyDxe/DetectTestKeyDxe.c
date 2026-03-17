/** @file
  Detects if PcdFmpDevicePkcs7CertBufferXdr contains a test key.

  Copyright (c) 2026, 3mdeb Sp. z o.o. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DetectTestKeyLib.h>

/**

  Check if PcdFmpDevicePkcs7CertBufferXdr contains a test key.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Regardless of the outcome.

**/
EFI_STATUS
EFIAPI
EntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DetectTestKey ();

  return EFI_SUCCESS;
}
