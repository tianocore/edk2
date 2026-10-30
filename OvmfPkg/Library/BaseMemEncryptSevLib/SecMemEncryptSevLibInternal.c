/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/AmdSevLib.h>
#include <Library/PcdLib.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Uefi/UefiBaseType.h>

/**
  Returns the SEV encryption mask.

  @return  The SEV pagtable encryption mask
**/
UINT64
EFIAPI
MemEncryptSevGetEncryptionMask (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  SevEsWorkArea = AmdSevGetSevEsWorkArea ();
  if (SevEsWorkArea == NULL) {
    return 0;
  }

  return SevEsWorkArea->EncryptionMask;
}

/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptSevLocateInitialSmramSaveStateMapPages (
  OUT UINTN  *BaseAddress,
  OUT UINTN  *NumberOfPages
  )
{
  return RETURN_UNSUPPORTED;
}
