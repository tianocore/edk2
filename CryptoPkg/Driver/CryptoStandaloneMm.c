/** @file
  Installs the EDK II Crypto SMM Protocol in Standalone MM.

  Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/SmmCrypto.h>

extern CONST EDKII_CRYPTO_PROTOCOL  mEdkiiCrypto;

/**
  The module Entry Point of the Crypto Standalone MM Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CryptoStandaloneMmEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_HANDLE  Handle;

  Handle = NULL;
  return gMmst->MmInstallProtocolInterface (
                  &Handle,
                  &gEdkiiSmmCryptoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (EDKII_CRYPTO_PROTOCOL *)&mEdkiiCrypto
                  );
}
