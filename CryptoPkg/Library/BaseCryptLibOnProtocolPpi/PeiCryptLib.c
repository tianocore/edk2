/** @file
  Implements the GetCryptoServices() API that retuns a pointer to the EDK II
  Crypto PPI.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/Crypto.h>

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI.  The layout of the PPI, DXE Protocol, and SMM Protocol are
  identical which allows the implementation of the BaseCryptLib functions that
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
