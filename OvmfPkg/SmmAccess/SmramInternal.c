/** @file

  Functions and types shared by the SMM accessor PEI and DXE modules.

  Copyright (C) 2015, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/AcpiS3Context.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>

#include "SmramInternal.h"

//
// The value of PcdQ35TsegMbytes is saved into this variable at module startup.
//
UINT16 mQ35TsegMbytes;

/**
  Save PcdQ35TsegMbytes into mQ35TsegMbytes.
**/
VOID
InitQ35TsegMbytes (
  VOID
  )
{
  mQ35TsegMbytes = PcdGet16 (PcdQ35TsegMbytes);
}

/**
  Read the MCH_SMRAM and ESMRAMC registers, and update the LockState and
  OpenState fields in the PEI_SMM_ACCESS_PPI / EFI_SMM_ACCESS2_PROTOCOL object,
  from the D_LCK and T_EN bits.

  PEI_SMM_ACCESS_PPI and EFI_SMM_ACCESS2_PROTOCOL member functions can rely on
  the LockState and OpenState fields being up-to-date on entry, and they need
  to restore the same invariant on exit, if they touch the bits in question.

  @param[out] LockState  Reflects the D_LCK bit on output; TRUE iff SMRAM is
                         locked.
  @param[out] OpenState  Reflects the inverse of the T_EN bit on output; TRUE
                         iff SMRAM is open.
**/
VOID
GetStates (
  OUT BOOLEAN *LockState,
  OUT BOOLEAN *OpenState
)
{
  UINT8 SmramVal, EsmramcVal;

  SmramVal   = PciRead8 (DRAMC_REGISTER_Q35 (MCH_SMRAM));
  EsmramcVal = PciRead8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC));

  *LockState = !!(SmramVal & MCH_SMRAM_D_LCK);
  *OpenState = !(EsmramcVal & MCH_ESMRAMC_T_EN);
}

//
// The functions below follow the PEI_SMM_ACCESS_PPI and
// EFI_SMM_ACCESS2_PROTOCOL member declarations. The PeiServices and This
// pointers are removed (TSEG doesn't depend on them), and so is the
// DescriptorIndex parameter (TSEG doesn't support range-wise locking).
//
// The LockState and OpenState members that are common to both
// PEI_SMM_ACCESS_PPI and EFI_SMM_ACCESS2_PROTOCOL are taken and updated in
// isolation from the rest of the (non-shared) members.
//

EFI_STATUS
SmramAccessOpen (
  OUT BOOLEAN *LockState,
  OUT BOOLEAN *OpenState
  )
{
  //
  // Open TSEG by clearing T_EN.
  //
  PciAnd8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC),
    (UINT8)((~(UINT32)MCH_ESMRAMC_T_EN) & 0xff));

  GetStates (LockState, OpenState);
  if (!*OpenState) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
SmramAccessClose (
  OUT BOOLEAN *LockState,
  OUT BOOLEAN *OpenState
  )
{
  //
  // Close TSEG by setting T_EN.
  //
  PciOr8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC), MCH_ESMRAMC_T_EN);

  GetStates (LockState, OpenState);
  if (*OpenState) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
SmramAccessLock (
  OUT    BOOLEAN *LockState,
  IN OUT BOOLEAN *OpenState
  )
{
  if (*OpenState) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close & lock TSEG by setting T_EN and D_LCK.
  //
  PciOr8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC), MCH_ESMRAMC_T_EN);
  PciOr8 (DRAMC_REGISTER_Q35 (MCH_SMRAM),   MCH_SMRAM_D_LCK);

  GetStates (LockState, OpenState);
  if (*OpenState || !*LockState) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
SmramAccessGetCapabilities (
  IN BOOLEAN                  LockState,
  IN BOOLEAN                  OpenState,
  IN OUT UINTN                *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR *SmramMap
  )
{
  UINTN  OriginalSize;
  UINT32 TsegMemoryBaseMb, TsegMemoryBase;
  UINT64 CommonRegionState;
  UINT8  TsegSizeBits;

  OriginalSize  = *SmramMapSize;
  *SmramMapSize = DescIdxCount * sizeof *SmramMap;
  if (OriginalSize < *SmramMapSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Read the TSEG Memory Base register.
  //
  TsegMemoryBaseMb = PciRead32 (DRAMC_REGISTER_Q35 (MCH_TSEGMB));
  TsegMemoryBase = (TsegMemoryBaseMb >> MCH_TSEGMB_MB_SHIFT) << 20;

  //
  // Precompute the region state bits that will be set for all regions.
  //
  CommonRegionState = (OpenState ? EFI_SMRAM_OPEN : EFI_SMRAM_CLOSED) |
                      (LockState ? EFI_SMRAM_LOCKED : 0) |
                      EFI_CACHEABLE;

  //
  // The first region hosts an SMM_S3_RESUME_STATE object. It is located at the
  // start of TSEG. We round up the size to whole pages, and we report it as
  // EFI_ALLOCATED, so that the SMM_CORE stays away from it.
  //
  SmramMap[DescIdxSmmS3ResumeState].PhysicalStart = TsegMemoryBase;
  SmramMap[DescIdxSmmS3ResumeState].CpuStart      = TsegMemoryBase;
  SmramMap[DescIdxSmmS3ResumeState].PhysicalSize  =
    EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (sizeof (SMM_S3_RESUME_STATE)));
  SmramMap[DescIdxSmmS3ResumeState].RegionState   =
    CommonRegionState | EFI_ALLOCATED;

  //
  // Get the TSEG size bits from the ESMRAMC register.
  //
  TsegSizeBits = PciRead8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC)) &
                 MCH_ESMRAMC_TSEG_MASK;

  //
  // The second region is the main one, following the first.
  //
  SmramMap[DescIdxMain].PhysicalStart =
    SmramMap[DescIdxSmmS3ResumeState].PhysicalStart +
    SmramMap[DescIdxSmmS3ResumeState].PhysicalSize;
  SmramMap[DescIdxMain].CpuStart = SmramMap[DescIdxMain].PhysicalStart;
  SmramMap[DescIdxMain].PhysicalSize =
    (TsegSizeBits == MCH_ESMRAMC_TSEG_8MB ? SIZE_8MB :
     TsegSizeBits == MCH_ESMRAMC_TSEG_2MB ? SIZE_2MB :
     TsegSizeBits == MCH_ESMRAMC_TSEG_1MB ? SIZE_1MB :
     mQ35TsegMbytes * SIZE_1MB) -
    SmramMap[DescIdxSmmS3ResumeState].PhysicalSize;
  SmramMap[DescIdxMain].RegionState = CommonRegionState;

  return EFI_SUCCESS;
}
