/** @file
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
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/Crypto.h>

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identicaly which allows the implementation of the BaseCryptLib functions that
  call through a Protocol/PPI to be shared for the PEI, DXE, and SMM
  implementations.

  This PEI implementation looks up the EDK II Crypto PPI and verifies the
  version each time a crypto service is called, so it is compatible with XIP
  PEIMs.
**/
VOID *
GetCryptoServices (
  VOID
  )
{
  EFI_STATUS        Status;
  EDKII_CRYPTO_PPI  *CryptoPpi;
  UINTN             Version;

  CryptoPpi = NULL;
  Status = PeiServicesLocatePpi (
             &gEdkiiCryptoPpiGuid,
             0,
             NULL,
             (VOID **)&CryptoPpi
             );
  if (EFI_ERROR (Status) || CryptoPpi == NULL) {
    DEBUG((DEBUG_ERROR, "[PeiCryptLib] Failed to locate Crypto PPI. Status = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (CryptoPpi != NULL);
    return NULL;
  }

  Version = CryptoPpi->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG((DEBUG_ERROR, "[PeiCryptLib] Crypto PPI unsupported version %d\n", Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    return NULL;
  }

  return (VOID *)CryptoPpi;
}
