/** @file
  Set a IDT entry for debug purpose

  Set a IDT entry for interrupt vector 3 for debug purpose for x64 platform

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "ScriptExecute.h"

//
// 8 extra pages for PF handler.
//
#define EXTRA_PAGE_TABLE_PAGES      8

#define IA32_PG_P                   BIT0
#define IA32_PG_RW                  BIT1
#define IA32_PG_PS                  BIT7

UINT64                              mPhyMask;
VOID                                *mOriginalHandler;
UINTN                               mPageFaultBuffer;
UINTN                               mPageFaultIndex = 0;
//
// Store the uplink information for each page being used.
//
UINT64                              *mPageFaultUplink[EXTRA_PAGE_TABLE_PAGES];

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
  IN IA32_IDT_GATE_DESCRIPTOR                   *IdtEntry
  )
{
  UINT32         RegEax;
  UINT8          PhysicalAddressBits;
  UINTN          PageFaultHandlerHookAddress;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits = 36;
  }
  mPhyMask = LShiftU64 (1, PhysicalAddressBits) - 1;
  mPhyMask &= (1ull << 48) - SIZE_4KB;

  //
  // Set Page Fault entry to catch >4G access
  //
  PageFaultHandlerHookAddress = (UINTN)PageFaultHandlerHook;
  mOriginalHandler = (VOID *)(UINTN)(LShiftU64 (IdtEntry->Bits.OffsetUpper, 32) + IdtEntry->Bits.OffsetLow + (IdtEntry->Bits.OffsetHigh << 16));
  IdtEntry->Bits.OffsetLow      = (UINT16)PageFaultHandlerHookAddress;
  IdtEntry->Bits.Selector       = (UINT16)AsmReadCs ();
  IdtEntry->Bits.Reserved_0     = 0;
  IdtEntry->Bits.GateType       = IA32_IDT_GATE_TYPE_INTERRUPT_32;
  IdtEntry->Bits.OffsetHigh     = (UINT16)(PageFaultHandlerHookAddress >> 16);
  IdtEntry->Bits.OffsetUpper    = (UINT32)(PageFaultHandlerHookAddress >> 32);
  IdtEntry->Bits.Reserved_1     = 0;

  if (mPage1GSupport) {
    mPageFaultBuffer = (UINTN)(AsmReadCr3 () & mPhyMask) + EFI_PAGES_TO_SIZE(2);
  }else {
    mPageFaultBuffer = (UINTN)(AsmReadCr3 () & mPhyMask) + EFI_PAGES_TO_SIZE(6);
  }
  ZeroMem (mPageFaultUplink, sizeof (mPageFaultUplink));
}

/**
  The function will check if current waking vector is long mode.

  @param  AcpiS3Context                 a pointer to a structure of ACPI_S3_CONTEXT

  @retval TRUE   Current context need long mode waking vector.
  @retval FALSE  Current context need not long mode waking vector.
**/
BOOLEAN
IsLongModeWakingVector (
  IN ACPI_S3_CONTEXT                *AcpiS3Context
  )
{
  EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;

  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *) ((UINTN) (AcpiS3Context->AcpiFacsTable));
  if ((Facs == NULL) ||
      (Facs->Signature != EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) ||
      ((Facs->FirmwareWakingVector == 0) && (Facs->XFirmwareWakingVector == 0)) ) {
    // Something wrong with FACS
    return FALSE;
  }
  if (Facs->XFirmwareWakingVector != 0) {
    if ((Facs->Version == EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        ((Facs->Flags & EFI_ACPI_4_0_64BIT_WAKE_SUPPORTED_F) != 0) &&
        ((Facs->Flags & EFI_ACPI_4_0_OSPM_64BIT_WAKE__F) != 0)) {
      // Both BIOS and OS wants 64bit vector
      if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
        return TRUE;
      }
    }
  }
  return FALSE;
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
  IA32_IDT_GATE_DESCRIPTOR                      *IdtEntry;
  IA32_DESCRIPTOR                               *IdtDescriptor;
  UINTN                                         S3DebugBuffer;
  EFI_STATUS                                    Status;

  //
  // Restore IDT for debug
  //
  IdtDescriptor = (IA32_DESCRIPTOR *) (UINTN) (AcpiS3Context->IdtrProfile);
  AsmWriteIdtr (IdtDescriptor);

  //
  // Setup the default CPU exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    //
    // Update IDT entry INT3 if the instruction is valid in it
    //
    S3DebugBuffer = (UINTN) (AcpiS3Context->S3DebugBufferAddress);
    if (*(UINTN *)S3DebugBuffer != (UINTN) -1) {
      IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (IA32_IDT_GATE_DESCRIPTOR)));
      IdtEntry->Bits.OffsetLow      = (UINT16)S3DebugBuffer;
      IdtEntry->Bits.Selector       = (UINT16)AsmReadCs ();
      IdtEntry->Bits.Reserved_0     = 0;
      IdtEntry->Bits.GateType       = IA32_IDT_GATE_TYPE_INTERRUPT_32;
      IdtEntry->Bits.OffsetHigh     = (UINT16)(S3DebugBuffer >> 16);
      IdtEntry->Bits.OffsetUpper    = (UINT32)(S3DebugBuffer >> 32);
      IdtEntry->Bits.Reserved_1     = 0;
    }
  );

  //
  // If both BIOS and OS wants long mode waking vector,
  // S3ResumePei should have established 1:1 Virtual to Physical identity mapping page table,
  // no need to hook page fault handler.
  //
  if (!IsLongModeWakingVector (AcpiS3Context)) {
    IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (14 * sizeof (IA32_IDT_GATE_DESCRIPTOR)));
    HookPageFaultHandler (IdtEntry);
  }
}

/**
  Acquire page for page fault.

  @param[in, out] Uplink        Pointer to up page table entry.

**/
VOID
AcquirePage (
  IN OUT UINT64                 *Uplink
  )
{
  UINTN             Address;

  Address = mPageFaultBuffer + EFI_PAGES_TO_SIZE (mPageFaultIndex);
  ZeroMem ((VOID *) Address, EFI_PAGES_TO_SIZE (1));

  //
  // Cut the previous uplink if it exists and wasn't overwritten.
  //
  if ((mPageFaultUplink[mPageFaultIndex] != NULL) && ((*mPageFaultUplink[mPageFaultIndex] & mPhyMask) == Address)) {
    *mPageFaultUplink[mPageFaultIndex] = 0;
  }

  //
  // Link & Record the current uplink.
  //
  *Uplink = Address | IA32_PG_P | IA32_PG_RW;
  mPageFaultUplink[mPageFaultIndex] = Uplink;

  mPageFaultIndex = (mPageFaultIndex + 1) % EXTRA_PAGE_TABLE_PAGES;
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
    AcquirePage (&PageTable[PTIndex]);
  }
  PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & mPhyMask);
  PTIndex = BitFieldRead64 (PFAddress, 30, 38);
  // PDPTE
  if (mPage1GSupport) {
    PageTable[PTIndex] = (PFAddress & ~((1ull << 30) - 1)) | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  } else {
    if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
      AcquirePage (&PageTable[PTIndex]);
    }
    PageTable = (UINT64*)(UINTN)(PageTable[PTIndex] & mPhyMask);
    PTIndex = BitFieldRead64 (PFAddress, 21, 29);
    // PD
    PageTable[PTIndex] = (PFAddress & ~((1ull << 21) - 1)) | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  }

  return TRUE;
}
