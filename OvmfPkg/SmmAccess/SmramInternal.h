/** @file

  Functions and types shared by the SMM accessor PEI and DXE modules.

  Copyright (C) 2015, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Pi/PiMultiPhase.h>

//
// We'll have two SMRAM ranges.
//
// The first is a tiny one that hosts an SMM_S3_RESUME_STATE object, to be
// filled in by the CPU SMM driver during normal boot, for the PEI instance of
// the LockBox library (which will rely on the object during S3 resume).
//
// The other SMRAM range is the main one, for the SMM core and the SMM drivers.
//
typedef enum {
  DescIdxSmmS3ResumeState = 0,
  DescIdxMain             = 1,
  DescIdxCount            = 2
} DESCRIPTOR_INDEX;

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
  OUT BOOLEAN *LockState,
  OUT BOOLEAN *OpenState
  );

EFI_STATUS
SmramAccessClose (
  OUT BOOLEAN *LockState,
  OUT BOOLEAN *OpenState
  );

EFI_STATUS
SmramAccessLock (
  OUT    BOOLEAN *LockState,
  IN OUT BOOLEAN *OpenState
  );

EFI_STATUS
SmramAccessGetCapabilities (
  IN BOOLEAN                  LockState,
  IN BOOLEAN                  OpenState,
  IN OUT UINTN                *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR *SmramMap
  );
