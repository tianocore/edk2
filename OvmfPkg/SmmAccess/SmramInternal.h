/** @file

  Functions and types shared by the SMM accessor PEI and DXE modules.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2024 Intel Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Pi/PiMultiPhase.h>

#include <Guid/SmramMemoryReserve.h>
#include <Library/HobLib.h>

//
// The value of PcdQ35TsegMbytes is saved into this variable at module startup.
//
extern UINT16  mQ35TsegMbytes;

/**
  Save PcdQ35TsegMbytes into mQ35TsegMbytes.
**/
VOID
InitQ35TsegMbytes (
  VOID
  );

/**
  Save PcdQ35SmramAtDefaultSmbase into mQ35SmramAtDefaultSmbase.
**/
VOID
InitQ35SmramAtDefaultSmbase (
  VOID
  );

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
  );

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
  );

EFI_STATUS
SmramAccessClose (
  OUT BOOLEAN  *LockState,
  OUT BOOLEAN  *OpenState
  );

EFI_STATUS
SmramAccessLock (
  OUT    BOOLEAN  *LockState,
  IN OUT BOOLEAN  *OpenState
  );

EFI_STATUS
SmramAccessGetCapabilities (
  IN OUT UINTN                 *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramMap
  );
