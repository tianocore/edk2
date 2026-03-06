/** @file
  Ia32-specific functionality for DxeLoad.

Copyright (c) 2006 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>
#include "VirtualMemory.h"
#include "UefiPayloadEntry.h"

#define STACK_SIZE       0x20000
#define IDT_ENTRY_COUNT  32

extern VOID  *mHobList;

typedef struct _X64_IDT_TABLE {
  //
  // Reserved 4 bytes preceding PeiService and IdtTable,
  // since IDT base address should be 8-byte alignment.
  //
  UINT32                     Reserved;
  CONST EFI_PEI_SERVICES     **PeiService;
  X64_IDT_GATE_DESCRIPTOR    IdtTable[IDT_ENTRY_COUNT];
} X64_IDT_TABLE;

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT  gGdtEntries[] = {
  /* selector { Global Segment Descriptor                              } */
  /* 0x00 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // null descriptor
  /* 0x08 */ {
    { 0xffff, 0, 0, 0x2, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear data segment descriptor
  /* 0x10 */ {
    { 0xffff, 0, 0, 0xf, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear code segment descriptor
  /* 0x18 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x20 */ {
    { 0xffff, 0, 0, 0xa, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x28 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
  /* 0x30 */ {
    { 0xffff, 0, 0, 0x2, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x38 */ {
    { 0xffff, 0, 0, 0xa, 1, 0, 1, 0xf, 0, 1, 0, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x40 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST IA32_DESCRIPTOR  gGdt = {
  sizeof (gGdtEntries) - 1,
  (UINTN)gGdtEntries
};

GLOBAL_REMOVE_IF_UNREFERENCED  IA32_DESCRIPTOR  gLidtDescriptor = {
  sizeof (X64_IDT_GATE_DESCRIPTOR) * IDT_ENTRY_COUNT - 1,
  0
};

/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.

   @param DxeCoreEntryPoint         The entry point of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  EFI_PHYSICAL_ADDRESS     BaseOfStack;
  EFI_PHYSICAL_ADDRESS     TopOfStack;
  UINTN                    PageTables;
  X64_IDT_GATE_DESCRIPTOR  *IdtTable;
  UINTN                    SizeOfTemplate;
  VOID                     *TemplateBase;
  EFI_PHYSICAL_ADDRESS     VectorAddress;
  UINT32                   Index;
  X64_IDT_TABLE            *IdtTableForX64;

  // Initialize floating point operating environment to be compliant with UEFI spec.
  InitializeFloatingPointUnits ();

  //
  // Mask off all legacy 8259 interrupt sources
  //
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0xFF);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0xFF);

  BaseOfStack = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != 0);

  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    //
    // Compute the top of the stack we were allocated, which is used to load X64 dxe core.
    // Pre-allocate a 32 bytes which confroms to x64 calling convention.
    //
    // The first four parameters to a function are passed in rcx, rdx, r8 and r9.
    // Any further parameters are pushed on the stack. Furthermore, space (4 * 8bytes) for the
    // register parameters is reserved on the stack, in case the called function
    // wants to spill them; this is important if the function is variadic.
    //
    TopOfStack = BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - 32;

    //
    //  x64 Calling Conventions requires that the stack must be aligned to 16 bytes
    //
    TopOfStack = (EFI_PHYSICAL_ADDRESS)(UINTN)ALIGN_POINTER (TopOfStack, 16);

    //
    // Load the GDT of Go64. Since the GDT of 32-bit Tiano locates in the BS_DATA
    // memory, it may be corrupted when copying FV to high-end memory
    //
    AsmWriteGdtr (&gGdt);
    //
    // Create page table and save PageMapLevel4 to CR3
    //
    PageTables = CreateIdentityMappingPageTables (BaseOfStack, STACK_SIZE, 0, 0);

    //
    // Paging might be already enabled. To avoid conflict configuration,
    // disable paging first anyway.
    //
    AsmWriteCr0 (AsmReadCr0 () & (~BIT31));
    AsmWriteCr3 (PageTables);

    //
    // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
    //
    UpdateStackHob (BaseOfStack, STACK_SIZE);

    SizeOfTemplate = AsmGetVectorTemplatInfo (&TemplateBase);

    VectorAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages (EFI_SIZE_TO_PAGES (sizeof (X64_IDT_TABLE) + SizeOfTemplate * IDT_ENTRY_COUNT));
    ASSERT (VectorAddress != 0);

    //
    // Store EFI_PEI_SERVICES** in the 4 bytes immediately preceding IDT to avoid that
    // it may not be gotten correctly after IDT register is re-written.
    //
    IdtTableForX64             = (X64_IDT_TABLE *)(UINTN)VectorAddress;
    IdtTableForX64->PeiService = NULL;

    VectorAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)(IdtTableForX64 + 1);
    IdtTable      = IdtTableForX64->IdtTable;
    for (Index = 0; Index < IDT_ENTRY_COUNT; Index++) {
      IdtTable[Index].Ia32IdtEntry.Bits.GateType   =  0x8e;
      IdtTable[Index].Ia32IdtEntry.Bits.Reserved_0 =  0;
      IdtTable[Index].Ia32IdtEntry.Bits.Selector   =  SYS_CODE64_SEL;

      IdtTable[Index].Ia32IdtEntry.Bits.OffsetLow  = (UINT16)VectorAddress;
      IdtTable[Index].Ia32IdtEntry.Bits.OffsetHigh = (UINT16)(RShiftU64 (VectorAddress, 16));
      IdtTable[Index].Offset32To63                 = (UINT32)(RShiftU64 (VectorAddress, 32));
      IdtTable[Index].Reserved                     = 0;

      CopyMem ((VOID *)(UINTN)VectorAddress, TemplateBase, SizeOfTemplate);
      AsmVectorFixup ((VOID *)(UINTN)VectorAddress, (UINT8)Index);

      VectorAddress += SizeOfTemplate;
    }

    gLidtDescriptor.Base = (UINTN)IdtTable;

    AsmWriteIdtr (&gLidtDescriptor);

    DEBUG ((
      DEBUG_INFO,
      "%a() Stack Base: 0x%llx, Stack Size: 0x%x\n",
      __func__,
      BaseOfStack,
      STACK_SIZE
      ));

    //
    // Go to Long Mode and transfer control to DxeCore.
    // Interrupts will not get turned on until the CPU AP is loaded.
    // Call x64 drivers passing in single argument, a pointer to the HOBs.
    //
    AsmEnablePaging64 (
      SYS_CODE64_SEL,
      DxeCoreEntryPoint,
      (EFI_PHYSICAL_ADDRESS)(UINTN)(HobList.Raw),
      0,
      TopOfStack
      );
  } else {
    // 32bit UEFI payload could be supported if required later.
    DEBUG ((DEBUG_ERROR, "NOT support 32bit UEFI payload\n"));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}
