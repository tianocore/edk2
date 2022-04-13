/** @file
  Map TPM MMIO range unencrypted when SEV-ES is active.
  Install gOvmfTpmMmioAccessiblePpiGuid unconditionally.

  Copyright (C) 2021, Advanced Micro Devices, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpmMmioRangeAccessible = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gOvmfTpmMmioAccessiblePpiGuid,
  NULL
};

/**
  The entry point for TPM MMIO range mapping driver.

  @param[in]  FileHandle   Handle of the file being invoked.
  @param[in]  PeiServices  Describes the list of possible PEI Services.

  @retval  EFI_ABORTED  No need to keep this PEIM resident
**/
EFI_STATUS
EFIAPI
TpmMmioSevDecryptPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  RETURN_STATUS  DecryptStatus;
  EFI_STATUS     Status;

  DEBUG ((DEBUG_INFO, "%a\n", __FUNCTION__));

  //
  // If SEV is active, MMIO succeeds against an encrypted physical address
  // because the nested page fault (NPF) that occurs on access does not
  // include the encryption bit in the guest physical address provided to the
  // hypervisor.
  //
  // If SEV-ES is active, MMIO would succeed against an encrypted physical
  // address because the #VC handler uses the virtual address (which is an
  // identity mapped physical address without the encryption bit) as the guest
  // physical address of the MMIO target in the VMGEXIT.
  //
  // However, if SEV-ES is active, before performing the actual MMIO, an
  // additional MMIO mitigation check is performed in the #VC handler to ensure
  // that MMIO is being done to/from an unencrypted address. To prevent guest
  // termination in this scenario, mark the range unencrypted ahead of access.
  //
  if (MemEncryptSevEsIsEnabled ()) {
    DEBUG ((
      DEBUG_INFO,
      "%a: mapping TPM MMIO address range unencrypted\n",
      __FUNCTION__
      ));

    DecryptStatus = MemEncryptSevClearMmioPageEncMask (
                      0,
                      FixedPcdGet64 (PcdTpmBaseAddress),
                      EFI_SIZE_TO_PAGES ((UINTN)0x5000)
                      );

    if (RETURN_ERROR (DecryptStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: failed to map TPM MMIO address range unencrypted\n",
        __FUNCTION__
        ));
      ASSERT_RETURN_ERROR (DecryptStatus);
    }
  }

  //
  // MMIO range available
  //
  Status = PeiServicesInstallPpi (&mTpmMmioRangeAccessible);
  ASSERT_EFI_ERROR (Status);

  return EFI_ABORTED;
}
