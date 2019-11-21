/** @file
SmmCryptLib.c

Copyright (c) 2019, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiSmm.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmCrypto.h>

EDKII_SMM_CRYPTO_PROTOCOL  *mSmmCryptoProtocol = NULL;

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identicaly which allows the implementation of the BaseCryptLib functions that
  call through a Protocol/PPI to be shared for the PEI, DXE, and SMM
  implementations.

  This SMM implementation returns the pointer to the EDK II SMM Crypto Protocol
  that was found in the library constructor SmmCryptLibConstructor().
**/
VOID *
GetCryptoServices (
  VOID
  )
{
  return (VOID *)mSmmCryptoProtocol;
}

/**
  Constructor looks up the EDK II SMM Crypto Protocol and verifies that it is
  not NULL and has a high enough version value to support all the BaseCryptLib
  functions.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval  EFI_SUCCESS    The EDK II SMM Crypto Protocol was found.
  @retval  EFI_NOT_FOUND  The EDK II SMM Crypto Protocol was not found.
**/
EFI_STATUS
EFIAPI
SmmCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Version;

  Status = gSmst->SmmLocateProtocol (
                    &gEdkiiSmmCryptoProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCryptoProtocol
                    );
  if (EFI_ERROR (Status) || mSmmCryptoProtocol == NULL) {
    DEBUG((DEBUG_ERROR, "[SmmCryptLib] Failed to locate Crypto SMM Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (mSmmCryptoProtocol != NULL);
    mSmmCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Version = mSmmCryptoProtocol->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG((DEBUG_ERROR, "[SmmCryptLib] Crypto SMM Protocol unsupported version %d\n", Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    mSmmCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
