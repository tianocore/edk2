/** @file
  Implements the GetCryptoServices() API that retuns a pointer to the EDK II
  Crypto Protocol.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Crypto.h>

EDKII_CRYPTO_PROTOCOL  *mCryptoProtocol = NULL;

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identical which allows the implementation of the BaseCryptLib functions that
  call through a Protocol/PPI to be shared for the PEI, DXE, and SMM
  implementations.

  This DXE implementation returns the pointer to the EDK II Crypto Protocol
  that was found in the library constructor DxeCryptLibConstructor().
**/
VOID *
GetCryptoServices (
  VOID
  )
{
  return (VOID *)mCryptoProtocol;
}

/**
  Locate the valid Crypto Protocol.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor executed correctly.
  @retval EFI_NOT_FOUND Found no valid Crypto Protocol.
**/
EFI_STATUS
EFIAPI
DxeCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Version;

  Status = gBS->LocateProtocol (
                  &gEdkiiCryptoProtocolGuid,
                  NULL,
                  (VOID **)&mCryptoProtocol
                  );

  if (EFI_ERROR (Status) || (mCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[DxeCryptLib] Failed to locate Crypto Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (mCryptoProtocol != NULL);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Version = mCryptoProtocol->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG ((DEBUG_ERROR, "[DxeCryptLib] Crypto Protocol unsupported version %d\n", Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
