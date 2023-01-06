/** @file
  Main SEC phase code.  Transitions to PEI.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/IoLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/LocalApicLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <IndustryStandard/Tdx.h>
#include <Library/TdxHelperLib.h>
#include <Library/CcProbeLib.h>
#include <Library/PeilessStartupLib.h>

#define SEC_IDT_ENTRY_COUNT  34

typedef struct _SEC_IDT_TABLE {
  EFI_PEI_SERVICES            *PeiService;
  IA32_IDT_GATE_DESCRIPTOR    IdtTable[SEC_IDT_ENTRY_COUNT];
} SEC_IDT_TABLE;

//
// Template of an IDT entry pointing to 10:FFFFFFE4h.
//
IA32_IDT_GATE_DESCRIPTOR  mIdtEntryTemplate = {
  {                                      // Bits
    0xffe4,                              // OffsetLow
    0x10,                                // Selector
    0x0,                                 // Reserved_0
    IA32_IDT_GATE_TYPE_INTERRUPT_32,     // GateType
    0xffff                               // OffsetHigh
  }
};

VOID
EFIAPI
SecCoreStartupWithStack (
  IN EFI_FIRMWARE_VOLUME_HEADER  *BootFv,
  IN VOID                        *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF  SecCoreData;
  SEC_IDT_TABLE         IdtTableInStack;
  IA32_DESCRIPTOR       IdtDescriptor;
  UINT32                Index;
  volatile UINT8        *Table;

  if (CcProbe () == CcGuestTypeIntelTdx) {
    //
    // From the security perspective all the external input should be measured before
    // it is consumed. TdHob and Configuration FV (Cfv) image are passed from VMM
    // and should be measured here.
    //
    if (EFI_ERROR (TdxHelperMeasureTdHob ())) {
      CpuDeadLoop ();
    }

    if (EFI_ERROR (TdxHelperMeasureCfvImage ())) {
      CpuDeadLoop ();
    }

    //
    // For Td guests, the memory map info is in TdHobLib. It should be processed
    // first so that the memory is accepted. Otherwise access to the unaccepted
    // memory will trigger tripple fault.
    //
    if (TdxHelperProcessTdHob () != EFI_SUCCESS) {
      CpuDeadLoop ();
    }
  }

  //
  // To ensure SMM can't be compromised on S3 resume, we must force re-init of
  // the BaseExtractGuidedSectionLib. Since this is before library contructors
  // are called, we must use a loop rather than SetMem.
  //
  Table = (UINT8 *)(UINTN)FixedPcdGet64 (PcdGuidedExtractHandlerTableAddress);
  for (Index = 0;
       Index < FixedPcdGet32 (PcdGuidedExtractHandlerTableSize);
       ++Index)
  {
    Table[Index] = 0;
  }

  //
  // Initialize IDT - Since this is before library constructors are called,
  // we use a loop rather than CopyMem.
  //
  IdtTableInStack.PeiService = NULL;

  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index++) {
    //
    // Declare the local variables that actually move the data elements as
    // volatile to prevent the optimizer from replacing this function with
    // the intrinsic memcpy()
    //
    CONST UINT8     *Src;
    volatile UINT8  *Dst;
    UINTN           Byte;

    Src = (CONST UINT8 *)&mIdtEntryTemplate;
    Dst = (volatile UINT8 *)&IdtTableInStack.IdtTable[Index];

    for (Byte = 0; Byte < sizeof (mIdtEntryTemplate); Byte++) {
      Dst[Byte] = Src[Byte];
    }
  }

  IdtDescriptor.Base  = (UINTN)&IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

  ProcessLibraryConstructorList (NULL, NULL);

  //
  // Load the IDTR.
  //
  AsmWriteIdtr (&IdtDescriptor);

  if (CcProbe () == CcGuestTypeIntelTdx) {
    //
    // InitializeCpuExceptionHandlers () should be called in Td guests so that
    // #VE exceptions can be handled correctly.
    //
    InitializeCpuExceptionHandlers (NULL);
  }

  DEBUG ((
    DEBUG_INFO,
    "SecCoreStartupWithStack(0x%x, 0x%x)\n",
    (UINT32)(UINTN)BootFv,
    (UINT32)(UINTN)TopOfCurrentStack
    ));

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();

  //
  // ASSERT that the Page Tables were set by the reset vector code to
  // the address we expect.
  //
  ASSERT (AsmReadCr3 () == (UINTN)PcdGet32 (PcdOvmfSecPageTablesBase));

  //
  // |-------------|       <-- TopOfCurrentStack
  // |   Stack     | 32k
  // |-------------|
  // |    Heap     | 32k
  // |-------------|       <-- SecCoreData.TemporaryRamBase
  //

  ASSERT (
    (UINTN)(PcdGet32 (PcdOvmfSecPeiTempRamBase) +
            PcdGet32 (PcdOvmfSecPeiTempRamSize)) ==
    (UINTN)TopOfCurrentStack
    );

  //
  // Initialize SEC hand-off state
  //
  SecCoreData.DataSize = sizeof (EFI_SEC_PEI_HAND_OFF);

  SecCoreData.TemporaryRamSize = (UINTN)PcdGet32 (PcdOvmfSecPeiTempRamSize);
  SecCoreData.TemporaryRamBase = (VOID *)((UINT8 *)TopOfCurrentStack - SecCoreData.TemporaryRamSize);

  SecCoreData.PeiTemporaryRamBase = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.StackBase = (UINT8 *)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize;
  SecCoreData.StackSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;

  //
  // Make sure the 8259 is masked before initializing the Debug Agent and the debug timer is enabled
  //
  IoWrite8 (0x21, 0xff);
  IoWrite8 (0xA1, 0xff);

  //
  // Initialize Local APIC Timer hardware and disable Local APIC Timer
  // interrupts before initializing the Debug Agent and the debug timer is
  // enabled.
  //
  InitializeApicTimer (0, MAX_UINT32, TRUE, 5);
  DisableApicTimerInterrupt ();

  PeilessStartup (&SecCoreData);

  ASSERT (FALSE);
  CpuDeadLoop ();
}
