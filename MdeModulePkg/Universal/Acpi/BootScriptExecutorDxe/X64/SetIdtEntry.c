/** @file
  Set a IDT entry for debug purpose

  Set a IDT entry for interrupt vector 3 for debug purpose for x64 platform

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "ScriptExecute.h"
//
// INTERRUPT_GATE_DESCRIPTOR and SetIdtEntry () are used to setup IDT to do debug
//

#pragma pack(1)

typedef struct {
  UINT16    Offset15To0;
  UINT16    SegmentSelector;
  UINT16    Attributes;
  UINT16    Offset31To16;
  UINT32    Offset63To32;
  UINT32    Reserved;
} INTERRUPT_GATE_DESCRIPTOR;

#define INTERRUPT_GATE_ATTRIBUTE   0x8e00

#pragma pack()

#define IA32_PG_P                   BIT0
#define IA32_PG_RW                  BIT1
#define IA32_PG_PS                  BIT7

UINT64                             mPhyMask;
BOOLEAN                            mPage1GSupport;
VOID                               *mOriginalHandler;
UINTN                              mS3NvsPageTableAddress;

/**
  Page fault handler.

**/
VOID
EFIAPI
PageFaultHandlerHook (
  VOID
  );

/**
  Hook IDT with our page fault handler so that the on-demand paging works on page fault.

  @param  IdtEntry  a pointer to IDT entry

**/
VOID
HookPageFaultHandler (
  IN INTERRUPT_GATE_DESCRIPTOR                     *IdtEntry
  )
{
  UINT32         RegEax;
  UINT32         RegEdx;

  AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
  mPhyMask = LShiftU64 (1, (UINT8)RegEax) - 1;
  mPhyMask &= (1ull << 48) - SIZE_4KB;

  mPage1GSupport = FALSE;
  if (PcdGetBool(PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        mPage1GSupport = TRUE;
      }
    }
  }

  //
  // Set Page Fault entry to catch >4G access
  //
  mOriginalHandler = (VOID *)(UINTN)(LShiftU64 (IdtEntry->Offset63To32, 32) + IdtEntry->Offset15To0 + (IdtEntry->Offset31To16 << 16));
  IdtEntry->Offset15To0     = (UINT16)((UINTN)PageFaultHandlerHook);
  IdtEntry->SegmentSelector = (UINT16)AsmReadCs ();
  IdtEntry->Attributes      = (UINT16)INTERRUPT_GATE_ATTRIBUTE;
  IdtEntry->Offset31To16    = (UINT16)((UINTN)PageFaultHandlerHook >> 16);
  IdtEntry->Offset63To32    = (UINT32)((UINTN)PageFaultHandlerHook >> 32);
  IdtEntry->Reserved        = 0;

  if (mPage1GSupport) {
    mS3NvsPageTableAddress = (UINTN)(AsmReadCr3 () & mPhyMask) + EFI_PAGES_TO_SIZE(2);
  }else {
    mS3NvsPageTableAddress = (UINTN)(AsmReadCr3 () & mPhyMask) + EFI_PAGES_TO_SIZE(6);
  }
}

/**
  Set a IDT entry for interrupt vector 3 for debug purpose.

  @param  AcpiS3Context  a pointer to a structure of ACPI_S3_CONTEXT

**/
VOID
SetIdtEntry (
  IN ACPI_S3_CONTEXT     *AcpiS3Context
  )
{
  INTERRUPT_GATE_DESCRIPTOR                     *IdtEntry;
  IA32_DESCRIPTOR                               *IdtDescriptor;
  UINTN                                         S3DebugBuffer;

  //
  // Restore IDT for debug
  //
  IdtDescriptor = (IA32_DESCRIPTOR *) (UINTN) (AcpiS3Context->IdtrProfile);
  IdtEntry = (INTERRUPT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (INTERRUPT_GATE_DESCRIPTOR)));
  S3DebugBuffer = (UINTN) (AcpiS3Context->S3DebugBufferAddress);

  IdtEntry->Offset15To0     = (UINT16)S3DebugBuffer;
  IdtEntry->SegmentSelector = (UINT16)AsmReadCs ();
  IdtEntry->Attributes      = (UINT16)INTERRUPT_GATE_ATTRIBUTE;
  IdtEntry->Offset31To16    = (UINT16)(S3DebugBuffer >> 16);
  IdtEntry->Offset63To32    = (UINT32)(S3DebugBuffer >> 32);
  IdtEntry->Reserved        = 0;

  IdtEntry = (INTERRUPT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (14 * sizeof (INTERRUPT_GATE_DESCRIPTOR)));
  HookPageFaultHandler (IdtEntry);

  AsmWriteIdtr (IdtDescriptor);
}

/**
  Get new page address.

  @param  PageNum  new page number needed

  @return new page address
**/
UINTN
GetNewPage (
  IN UINTN  PageNum
  )
{
  UINTN  NewPage;
  NewPage = mS3NvsPageTableAddress;
  ZeroMem ((VOID *)NewPage, EFI_PAGES_TO_SIZE(PageNum));
  mS3NvsPageTableAddress += EFI_PAGES_TO_SIZE(PageNum);
  return NewPage;
}

/**
  The page fault handler that on-demand read >4G memory/MMIO.
  
  @retval TRUE     The page fault is correctly handled.
  @retval FALSE    The page fault is not handled and is passed through to original handler.

**/
BOOLEAN
EFIAPI
PageFaultHandler (
  VOID
  )
{
  UINT64         *PageTable;
  UINT64         PFAddress;
  UINTN          PTIndex;

  PFAddress = AsmReadCr2 ();
  DEBUG ((EFI_D_ERROR, "BootScript - PageFaultHandler: Cr2 - %lx\n", PFAddress));

  if (PFAddress >= mPhyMask + SIZE_4KB) {
    return FALSE;
  }
  PFAddress &= mPhyMask;

  PageTable = (UINT64*)(UINTN)(AsmReadCr3 () & mPhyMask);

  PTIndex = BitFieldRead64 (PFAddress, 39, 47);
  // PML4E
  if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
    PageTable[PTIndex] = GetNewPage (1) | IA32_PG_P | IA32_PG_RW;
  }
  PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & mPhyMask);
  PTIndex = BitFieldRead64 (PFAddress, 30, 38);
  // PDPTE
  if (mPage1GSupport) {
    PageTable[PTIndex] = PFAddress | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  } else {
    if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
      PageTable[PTIndex] = GetNewPage (1) | IA32_PG_P | IA32_PG_RW;
    }
    PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & mPhyMask);
    PTIndex = BitFieldRead64 (PFAddress, 21, 29);
    // PD
    PageTable[PTIndex] = PFAddress | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  }

  return TRUE;
}
