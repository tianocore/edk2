/** @file
The version of the Crypto wrapper Library for DXE

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
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Crypto.h>

EDKII_CRYPTO_PROTOCOL  *mCryptoProtocol = NULL;

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identicaly which allows the implementation of the BaseCryptLib functions that
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

  if (EFI_ERROR (Status) || mCryptoProtocol == NULL) {
    DEBUG((DEBUG_ERROR, "[DxeCryptLib] Failed to locate Crypto Protocol. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (mCryptoProtocol != NULL);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Version = mCryptoProtocol->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG((DEBUG_ERROR, "[DxeCryptLib] Crypto Protocol unsupported version %d\n", Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
