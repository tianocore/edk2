/** @file
  Installs the EDK II Crypto PPI.  If this PEIM is dispatched before memory is
  discovered, the RegisterForShadow() feature is used to reload this PEIM into
  memory after memory is discovered.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Ppi/Crypto.h>

extern CONST EDKII_CRYPTO_PROTOCOL  mEdkiiCrypto;

CONST EFI_PEI_PPI_DESCRIPTOR  mEdkiiCryptoPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiCryptoPpiGuid,
  (EDKII_CRYPTO_PPI *)&mEdkiiCrypto
};

/**
Entry to CryptoPeiEntry.

@param FileHandle   The image handle.
@param PeiServices  The PEI services table.

@retval Status      From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
CryptoPeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  VOID                    *MemoryDiscoveredPpi;
  EDKII_CRYPTO_PPI        *EdkiiCryptoPpi;
  EFI_PEI_PPI_DESCRIPTOR  *EdkiiCryptoPpiDescriptor;

  //
  // Not all Open SSL services support XIP due to use of global variables.
  // Use gEfiPeiMemoryDiscoveredPpiGuid to detect Pre-Mem and Post-Mem and
  // always shadow this module in memory in Post-Mem.
  //
  Status = PeiServicesLocatePpi (
             &gEfiPeiMemoryDiscoveredPpiGuid,
             0,
             NULL,
             (VOID **)&MemoryDiscoveredPpi
             );
  if (Status == EFI_NOT_FOUND) {
    //
    // CryptoPei is dispatched before gEfiPeiMemoryDiscoveredPpiGuid
    //
    Status = PeiServicesRegisterForShadow (FileHandle);
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      //
      // First CryptoPpi installation. CryptoPei could come from memory or flash
      // it will be re-installed after gEfiPeiMemoryDiscoveredPpiGuid
      //
      DEBUG ((DEBUG_INFO, "CryptoPeiEntry: Install Pre-Memory Crypto PPI\n"));
      Status = PeiServicesInstallPpi (&mEdkiiCryptoPpiList);
      ASSERT_EFI_ERROR (Status);
    }
  } else if (Status == EFI_SUCCESS) {
    //
    // CryptoPei is dispatched after gEfiPeiMemoryDiscoveredPpiGuid
    //
    Status = PeiServicesLocatePpi (
               &gEdkiiCryptoPpiGuid,
               0,
               &EdkiiCryptoPpiDescriptor,
               (VOID **)&EdkiiCryptoPpi
               );
    if (!EFI_ERROR (Status)) {
      //
      // CryptoPei was also dispatched before gEfiPeiMemoryDiscoveredPpiGuid
      //
      DEBUG ((DEBUG_INFO, "CryptoPeiEntry: ReInstall Post-Memmory Crypto PPI\n"));
      Status = PeiServicesReInstallPpi (
                 EdkiiCryptoPpiDescriptor,
                 &mEdkiiCryptoPpiList
                 );
      ASSERT_EFI_ERROR (Status);
    } else {
      DEBUG ((DEBUG_INFO, "CryptoPeiEntry: Install Post-Memmory Crypto PPI\n"));
      Status = PeiServicesInstallPpi (&mEdkiiCryptoPpiList);
    }
  } else {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
