/** @file
  Installs the EDK II Crypto Protocol

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Crypto.h>

extern CONST EDKII_CRYPTO_PROTOCOL  mEdkiiCrypto;

/**
  The module Entry Point of the Crypto Dxe Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CryptoDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEdkiiCryptoProtocolGuid,
                (EDKII_CRYPTO_PROTOCOL *)&mEdkiiCrypto,
                NULL
                );
}
