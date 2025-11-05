/** @file
Provides services to access SMRAM Save State Map

Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>

#include <Library/SmmCpuFeaturesLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>

#include "PiSmmCpuCommon.h"

typedef struct {
  UINT64    Signature;                                      // Offset 0x00
  UINT16    Reserved1;                                      // Offset 0x08
  UINT16    Reserved2;                                      // Offset 0x0A
  UINT16    Reserved3;                                      // Offset 0x0C
  UINT16    SmmCs;                                          // Offset 0x0E
  UINT16    SmmDs;                                          // Offset 0x10
  UINT16    SmmSs;                                          // Offset 0x12
  UINT16    SmmOtherSegment;                                // Offset 0x14
  UINT16    Reserved4;                                      // Offset 0x16
  UINT64    Reserved5;                                      // Offset 0x18
  UINT64    Reserved6;                                      // Offset 0x20
  UINT64    Reserved7;                                      // Offset 0x28
  UINT64    SmmGdtPtr;                                      // Offset 0x30
  UINT32    SmmGdtSize;                                     // Offset 0x38
  UINT32    Reserved8;                                      // Offset 0x3C
  UINT64    Reserved9;                                      // Offset 0x40
  UINT64    Reserved10;                                     // Offset 0x48
  UINT16    Reserved11;                                     // Offset 0x50
  UINT16    Reserved12;                                     // Offset 0x52
  UINT32    Reserved13;                                     // Offset 0x54
  UINT64    Reserved14;                                     // Offset 0x58
} PROCESSOR_SMM_DESCRIPTOR;

extern CONST PROCESSOR_SMM_DESCRIPTOR  gcPsd;

//
// EFER register LMA bit
//
#define LMA  BIT10

///
/// Variables from SMI Handler
///
X86_ASSEMBLY_PATCH_LABEL  gPatchSmbase;
X86_ASSEMBLY_PATCH_LABEL  gPatchSmiStack;
X86_ASSEMBLY_PATCH_LABEL  gPatchSmiCr3;
extern volatile UINT8     gcSmiHandlerTemplate[];
extern CONST UINT16       gcSmiHandlerSize;

//
// Variables used by SMI Handler
//
IA32_DESCRIPTOR  gSmiHandlerIdtr;

///
/// The mode of the CPU at the time an SMI occurs
///
UINT8  mSmmSaveStateRegisterLma;

/**
  Get the size of the SMI Handler in bytes.

  @retval The size, in bytes, of the SMI Handler.

**/
UINTN
EFIAPI
GetSmiHandlerSize (
  VOID
  )
{
  UINTN  Size;

  Size = SmmCpuFeaturesGetSmiHandlerSize ();
  if (Size != 0) {
    return Size;
  }

  return gcSmiHandlerSize;
}

/**
  Install the SMI handler for the CPU specified by CpuIndex.  This function
  is called by the CPU that was elected as monarch during System Management
  Mode initialization.

  @param[in] CpuIndex   The index of the CPU to install the custom SMI handler.
                        The value must be between 0 and the NumberOfCpus field
                        in the System Management System Table (SMST).
  @param[in] SmBase     The SMBASE address for the CPU specified by CpuIndex.
  @param[in] SmiStack   The stack to use when an SMI is processed by the
                        the CPU specified by CpuIndex.
  @param[in] StackSize  The size, in bytes, if the stack used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtBase    The base address of the GDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] GdtSize    The size, in bytes, of the GDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtBase    The base address of the IDT to use when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] IdtSize    The size, in bytes, of the IDT used when an SMI is
                        processed by the CPU specified by CpuIndex.
  @param[in] Cr3        The base address of the page tables to use when an SMI
                        is processed by the CPU specified by CpuIndex.
**/
VOID
EFIAPI
InstallSmiHandler (
  IN UINTN   CpuIndex,
  IN UINT32  SmBase,
  IN VOID    *SmiStack,
  IN UINTN   StackSize,
  IN UINTN   GdtBase,
  IN UINTN   GdtSize,
  IN UINTN   IdtBase,
  IN UINTN   IdtSize,
  IN UINT32  Cr3
  )
{
  PROCESSOR_SMM_DESCRIPTOR  *Psd;
  UINT32                    CpuSmiStack;

  //
  // Initialize PROCESSOR_SMM_DESCRIPTOR
  //
  Psd = (PROCESSOR_SMM_DESCRIPTOR *)(VOID *)((UINTN)SmBase + SMM_PSD_OFFSET);
  CopyMem (Psd, &gcPsd, sizeof (gcPsd));
  Psd->SmmGdtPtr  = (UINT64)GdtBase;
  Psd->SmmGdtSize = (UINT32)GdtSize;

  if (SmmCpuFeaturesGetSmiHandlerSize () != 0) {
    //
    // Install SMI handler provided by library
    //
    SmmCpuFeaturesInstallSmiHandler (
      CpuIndex,
      SmBase,
      SmiStack,
      StackSize,
      GdtBase,
      GdtSize,
      IdtBase,
      IdtSize,
      Cr3
      );
    return;
  }

  InitShadowStack (CpuIndex, (VOID *)((UINTN)SmiStack + StackSize));

  //
  // Initialize values in template before copy
  //
  CpuSmiStack = (UINT32)((UINTN)SmiStack + StackSize - sizeof (UINTN));
  PatchInstructionX86 (gPatchSmiStack, CpuSmiStack, 4);
  PatchInstructionX86 (gPatchSmiCr3, Cr3, 4);
  PatchInstructionX86 (gPatchSmbase, SmBase, 4);
  gSmiHandlerIdtr.Base  = IdtBase;
  gSmiHandlerIdtr.Limit = (UINT16)(IdtSize - 1);

  //
  // Set the value at the top of the CPU stack to the CPU Index
  //
  *(UINTN *)(UINTN)CpuSmiStack = CpuIndex;

  //
  // Copy template to CPU specific SMI handler location
  //
  CopyMem (
    (VOID *)((UINTN)SmBase + SMM_HANDLER_OFFSET),
    (VOID *)gcSmiHandlerTemplate,
    gcSmiHandlerSize
    );
}
