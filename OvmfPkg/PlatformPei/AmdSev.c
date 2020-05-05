/**@file
  Initialize Secure Encrypted Virtualization (SEV) support

  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
//
// The package level header files this module uses
//
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <PiPei.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Amd/Msr.h>
#include <Register/Cpuid.h>
#include <Register/Intel/SmramSaveStateMap.h>

#include "Platform.h"

/**

  Initialize SEV-ES support if running as an SEV-ES guest.

  **/
STATIC
VOID
AmdSevEsInitialize (
  VOID
  )
{
  VOID              *GhcbBase;
  PHYSICAL_ADDRESS  GhcbBasePa;
  UINTN             GhcbPageCount, PageCount;
  RETURN_STATUS     PcdStatus, DecryptStatus;
  IA32_DESCRIPTOR   Gdtr;
  VOID              *Gdt;

  if (!MemEncryptSevEsIsEnabled ()) {
    return;
  }

  PcdStatus = PcdSetBoolS (PcdSevEsIsEnabled, TRUE);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // Allocate GHCB and per-CPU variable pages.
  //   Since the pages must survive across the UEFI to OS transition
  //   make them reserved.
  //
  GhcbPageCount = mMaxCpuCount * 2;
  GhcbBase = AllocateReservedPages (GhcbPageCount);
  ASSERT (GhcbBase != NULL);

  GhcbBasePa = (PHYSICAL_ADDRESS)(UINTN) GhcbBase;

  //
  // Each vCPU gets two consecutive pages, the first is the GHCB and the
  // second is the per-CPU variable page. Loop through the allocation and
  // only clear the encryption mask for the GHCB pages.
  //
  for (PageCount = 0; PageCount < GhcbPageCount; PageCount += 2) {
    DecryptStatus = MemEncryptSevClearPageEncMask (
      0,
      GhcbBasePa + EFI_PAGES_TO_SIZE (PageCount),
      1,
      TRUE
      );
    ASSERT_RETURN_ERROR (DecryptStatus);
  }

  ZeroMem (GhcbBase, EFI_PAGES_TO_SIZE (GhcbPageCount));

  PcdStatus = PcdSet64S (PcdGhcbBase, GhcbBasePa);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdGhcbSize, EFI_PAGES_TO_SIZE (GhcbPageCount));
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((DEBUG_INFO,
    "SEV-ES is enabled, %lu GHCB pages allocated starting at 0x%p\n",
    (UINT64)GhcbPageCount, GhcbBase));

  AsmWriteMsr64 (MSR_SEV_ES_GHCB, GhcbBasePa);

  //
  // The SEV support will clear the C-bit from non-RAM areas.  The early GDT
  // lives in a non-RAM area, so when an exception occurs (like a #VC) the GDT
  // will be read as un-encrypted even though it was created before the C-bit
  // was cleared (encrypted). This will result in a failure to be able to
  // handle the exception.
  //
  AsmReadGdtr (&Gdtr);

  Gdt = AllocatePages (EFI_SIZE_TO_PAGES ((UINTN) Gdtr.Limit + 1));
  ASSERT (Gdt != NULL);

  CopyMem (Gdt, (VOID *) Gdtr.Base, Gdtr.Limit + 1);
  Gdtr.Base = (UINTN) Gdt;
  AsmWriteGdtr (&Gdtr);
}

/**

  Function checks if SEV support is available, if present then it sets
  the dynamic PcdPteMemoryEncryptionAddressOrMask with memory encryption mask.

  **/
VOID
AmdSevInitialize (
  VOID
  )
{
  CPUID_MEMORY_ENCRYPTION_INFO_EBX  Ebx;
  UINT64                            EncryptionMask;
  RETURN_STATUS                     PcdStatus;

  //
  // Check if SEV is enabled
  //
  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // CPUID Fn8000_001F[EBX] Bit 0:5 (memory encryption bit position)
  //
  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, NULL, &Ebx.Uint32, NULL, NULL);
  EncryptionMask = LShiftU64 (1, Ebx.Bits.PtePosBits);

  //
  // Set Memory Encryption Mask PCD
  //
  PcdStatus = PcdSet64S (PcdPteMemoryEncryptionAddressOrMask, EncryptionMask);
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((DEBUG_INFO, "SEV is enabled (mask 0x%lx)\n", EncryptionMask));

  //
  // Set Pcd to Deny the execution of option ROM when security
  // violation.
  //
  PcdStatus = PcdSet32S (PcdOptionRomImageVerificationPolicy, 0x4);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // When SMM is required, cover the pages containing the initial SMRAM Save
  // State Map with a memory allocation HOB:
  //
  // There's going to be a time interval between our decrypting those pages for
  // SMBASE relocation and re-encrypting the same pages after SMBASE
  // relocation. We shall ensure that the DXE phase stay away from those pages
  // until after re-encryption, in order to prevent an information leak to the
  // hypervisor.
  //
  if (FeaturePcdGet (PcdSmmSmramRequire) && (mBootMode != BOOT_ON_S3_RESUME)) {
    RETURN_STATUS LocateMapStatus;
    UINTN         MapPagesBase;
    UINTN         MapPagesCount;

    LocateMapStatus = MemEncryptSevLocateInitialSmramSaveStateMapPages (
                        &MapPagesBase,
                        &MapPagesCount
                        );
    ASSERT_RETURN_ERROR (LocateMapStatus);

    if (mQ35SmramAtDefaultSmbase) {
      //
      // The initial SMRAM Save State Map has been covered as part of a larger
      // reserved memory allocation in InitializeRamRegions().
      //
      ASSERT (SMM_DEFAULT_SMBASE <= MapPagesBase);
      ASSERT (
        (MapPagesBase + EFI_PAGES_TO_SIZE (MapPagesCount) <=
         SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE)
        );
    } else {
      BuildMemoryAllocationHob (
        MapPagesBase,                      // BaseAddress
        EFI_PAGES_TO_SIZE (MapPagesCount), // Length
        EfiBootServicesData                // MemoryType
        );
    }
  }

  //
  // Check and perform SEV-ES initialization if required.
  //
  AmdSevEsInitialize ();
}
