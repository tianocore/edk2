/** @file

  Functions and types shared by the SMM accessor PEI and DXE modules.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2024 Intel Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>

#include "SmramInternal.h"

//
// The value of PcdQ35TsegMbytes is saved into this variable at module startup.
//
UINT16  mQ35TsegMbytes;

//
// The value of PcdQ35SmramAtDefaultSmbase is saved into this variable at
// module startup.
//
STATIC BOOLEAN  mQ35SmramAtDefaultSmbase;

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
  Save PcdQ35SmramAtDefaultSmbase into mQ35SmramAtDefaultSmbase.
**/
VOID
InitQ35SmramAtDefaultSmbase (
  VOID
  )
{
  mQ35SmramAtDefaultSmbase = PcdGetBool (PcdQ35SmramAtDefaultSmbase);
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
  OUT BOOLEAN  *LockState,
  OUT BOOLEAN  *OpenState
  )
{
  UINT8  SmramVal, EsmramcVal;

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
  OUT BOOLEAN  *LockState,
  OUT BOOLEAN  *OpenState
  )
{
  //
  // Open TSEG by clearing T_EN.
  //
  PciAnd8 (
    DRAMC_REGISTER_Q35 (MCH_ESMRAMC),
    (UINT8)((~(UINT32)MCH_ESMRAMC_T_EN) & 0xff)
    );

  GetStates (LockState, OpenState);
  if (!*OpenState) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SmramAccessClose (
  OUT BOOLEAN  *LockState,
  OUT BOOLEAN  *OpenState
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
  OUT    BOOLEAN  *LockState,
  IN OUT BOOLEAN  *OpenState
  )
{
  if (*OpenState) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close & lock TSEG by setting T_EN and D_LCK.
  //
  PciOr8 (DRAMC_REGISTER_Q35 (MCH_ESMRAMC), MCH_ESMRAMC_T_EN);
  PciOr8 (DRAMC_REGISTER_Q35 (MCH_SMRAM), MCH_SMRAM_D_LCK);

  //
  // Close & lock the SMRAM at the default SMBASE, if it exists.
  //
  if (mQ35SmramAtDefaultSmbase) {
    PciWrite8 (
      DRAMC_REGISTER_Q35 (MCH_DEFAULT_SMBASE_CTL),
      MCH_DEFAULT_SMBASE_LCK
      );
  }

  GetStates (LockState, OpenState);
  if (*OpenState || !*LockState) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SmramAccessGetCapabilities (
  IN OUT UINTN                 *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramMap
  )
{
  UINTN                           BufferSize;
  EFI_HOB_GUID_TYPE               *GuidHob;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock;
  UINTN                           Index;

  //
  // Get Hob list
  //
  GuidHob         = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock);

  BufferSize = DescriptorBlock->NumberOfSmmReservedRegions * sizeof (EFI_SMRAM_DESCRIPTOR);

  if (*SmramMapSize < BufferSize) {
    *SmramMapSize = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Update SmramMapSize to real return SMRAM map size
  //
  *SmramMapSize = BufferSize;

  //
  // Use the hob to publish SMRAM capabilities
  //
  for (Index = 0; Index < DescriptorBlock->NumberOfSmmReservedRegions; Index++) {
    SmramMap[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    SmramMap[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    SmramMap[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    SmramMap[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
  }

  return EFI_SUCCESS;
}
