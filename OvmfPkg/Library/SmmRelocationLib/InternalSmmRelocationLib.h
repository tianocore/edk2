/** @file
  SMM Relocation Lib for each processor.

  This Lib produces the SMM_BASE_HOB in HOB database which tells
  the PiSmmCpuDxeSmm driver (runs at a later phase) about the new
  SMBASE for each processor. PiSmmCpuDxeSmm driver installs the
  SMI handler at the SMM_BASE_HOB.SmBase[Index]+0x8000 for processor
  Index.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_SMM_RELOCATION_LIB_H_
#define INTERNAL_SMM_RELOCATION_LIB_H_

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/SmmRelocationLib.h>
#include <Guid/SmramMemoryReserve.h>
#include <Guid/SmmBaseHob.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/SmramSaveStateMap.h>
#include <Protocol/MmCpu.h>

extern IA32_DESCRIPTOR  gcSmmInitGdtr;
extern CONST UINT16     gcSmmInitSize;
extern CONST UINT8      gcSmmInitTemplate[];

X86_ASSEMBLY_PATCH_LABEL  gPatchSmmInitCr0;
X86_ASSEMBLY_PATCH_LABEL  gPatchSmmInitCr3;
X86_ASSEMBLY_PATCH_LABEL  gPatchSmmInitCr4;
X86_ASSEMBLY_PATCH_LABEL  gPatchSmmInitStack;

//
// The size 0x20 must be bigger than
// the size of template code of SmmInit. Currently,
// the size of SmmInit requires the 0x16 Bytes buffer
// at least.
//
#define BACK_BUF_SIZE  0x20

#define CR4_CET_ENABLE  BIT23

//
// EFER register LMA bit
//
#define LMA  BIT10

/**
  This function configures the SmBase on the currently executing CPU.

  @param[in]     SmBase             The SmBase on the currently executing CPU.

**/
VOID
EFIAPI
ConfigureSmBase (
  IN UINT64  SmBase
  );

/**
  Semaphore operation for all processor relocate SMMBase.
**/
VOID
EFIAPI
SmmRelocationSemaphoreComplete (
  VOID
  );

/**
  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.  The mode of code executed immediately after RSM must be
  detected, and the appropriate hook must be selected.  Always clear the auto
  HALT restart flag if it is set.

  @param[in,out] CpuState                 Pointer to SMRAM Save State Map for the
                                          currently executing CPU.
  @param[in]     NewInstructionPointer32  Instruction pointer to use if resuming to
                                          32-bit mode from 64-bit SMM.
  @param[in]     NewInstructionPointer    Instruction pointer to use if resuming to
                                          same mode as SMM.

  @retval The value of the original instruction pointer before it was hooked.

**/
UINT64
EFIAPI
HookReturnFromSmm (
  IN OUT SMRAM_SAVE_STATE_MAP  *CpuState,
  IN     UINT64                NewInstructionPointer32,
  IN     UINT64                NewInstructionPointer
  );

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param[in] RebasedFlag  A pointer to a flag that is set to TRUE
                          immediately after AP exits SMM.

**/
VOID
SemaphoreHook (
  IN volatile BOOLEAN  *RebasedFlag
  );

/**
  This function fixes up the address of the global variable or function
  referred in SmmInit assembly files to be the absolute address.
**/
VOID
EFIAPI
SmmInitFixupAddress (
  );

#endif
