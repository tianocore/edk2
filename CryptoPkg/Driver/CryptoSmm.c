/** @file
  Installs the EDK II Crypto SMM Protocol

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmCrypto.h>

extern CONST EDKII_CRYPTO_PROTOCOL  mEdkiiCrypto;

/**
  The module Entry Point of the Crypto SMM Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CryptoSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  Handle;

  Handle = NULL;
  return gSmst->SmmInstallProtocolInterface (
                  &Handle,
                  &gEdkiiSmmCryptoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (EDKII_CRYPTO_PROTOCOL *)&mEdkiiCrypto
                  );
}
