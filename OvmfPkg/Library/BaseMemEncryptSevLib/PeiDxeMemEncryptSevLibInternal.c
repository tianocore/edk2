/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Register/QemuSmramSaveStateMap.h>
#include <Register/SmramSaveStateMap.h>
#include <Uefi/UefiBaseType.h>

#include "PeiDxeMemEncryptSevLibInternal.h"

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
  UINTN  MapStart;
  UINTN  MapEnd;
  UINTN  MapPagesStart; // MapStart rounded down to page boundary
  UINTN  MapPagesEnd;   // MapEnd rounded up to page boundary
  UINTN  MapPagesSize;  // difference between MapPagesStart and MapPagesEnd

  if (!FeaturePcdGet (PcdSmmSmramRequire)) {
    return RETURN_UNSUPPORTED;
  }

  MapStart      = SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET;
  MapEnd        = MapStart + sizeof (QEMU_SMRAM_SAVE_STATE_MAP);
  MapPagesStart = MapStart & ~(UINTN)EFI_PAGE_MASK;
  MapPagesEnd   = ALIGN_VALUE (MapEnd, EFI_PAGE_SIZE);
  MapPagesSize  = MapPagesEnd - MapPagesStart;

  ASSERT ((MapPagesSize & EFI_PAGE_MASK) == 0);

  *BaseAddress   = MapPagesStart;
  *NumberOfPages = MapPagesSize >> EFI_PAGE_SHIFT;

  return RETURN_SUCCESS;
}

/**
  Figures out if we are running inside KVM HVM and
  KVM HVM supports SEV Live Migration feature.

  @retval TRUE           SEV live migration is supported.
  @retval FALSE          SEV live migration is not supported.
**/
BOOLEAN
EFIAPI
KvmDetectSevLiveMigrationFeature (
  VOID
  )
{
  CHAR8   Signature[13];
  UINT32  mKvmLeaf;
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;

  Signature[12] = '\0';
  for (mKvmLeaf = 0x40000000; mKvmLeaf < 0x40010000; mKvmLeaf += 0x100) {
    AsmCpuid (
      mKvmLeaf,
      NULL,
      (UINT32 *)&Signature[0],
      (UINT32 *)&Signature[4],
      (UINT32 *)&Signature[8]
      );

    if (AsciiStrCmp (Signature, "KVMKVMKVM") == 0) {
      DEBUG ((
        DEBUG_INFO,
        "%a: KVM Detected, signature = %a\n",
        __FUNCTION__,
        Signature
        ));

      RegEax = mKvmLeaf + 1;
      RegEcx = 0;
      AsmCpuid (mKvmLeaf + 1, &RegEax, &RegEbx, &RegEcx, &RegEdx);
      if ((RegEax & KVM_FEATURE_MIGRATION_CONTROL) != 0) {
        DEBUG ((
          DEBUG_INFO,
          "%a: SEV Live Migration feature supported\n",
          __FUNCTION__
          ));

        return TRUE;
      }
    }
  }

  return FALSE;
}
