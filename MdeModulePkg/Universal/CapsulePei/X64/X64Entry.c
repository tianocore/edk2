/** @file
  The X64 entrypoint is used to process capsule in long mode.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugAgentLib.h>
#include "CommonHeader.h"

#define EXCEPTION_VECTOR_NUMBER  0x22

#define IA32_PG_P   BIT0
#define IA32_PG_RW  BIT1
#define IA32_PG_PS  BIT7

typedef struct _PAGE_FAULT_CONTEXT {
  BOOLEAN    Page1GSupport;
  UINT64     PhyMask;
  UINTN      PageFaultBuffer;
  UINTN      PageFaultIndex;
  UINT64     AddressEncMask;
  //
  // Store the uplink information for each page being used.
  //
  UINT64     *PageFaultUplink[EXTRA_PAGE_TABLE_PAGES];
  VOID       *OriginalHandler;
} PAGE_FAULT_CONTEXT;

typedef struct _PAGE_FAULT_IDT_TABLE {
  PAGE_FAULT_CONTEXT          PageFaultContext;
  IA32_IDT_GATE_DESCRIPTOR    IdtEntryTable[EXCEPTION_VECTOR_NUMBER];
} PAGE_FAULT_IDT_TABLE;

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

  @param[in, out] IdtEntry          Pointer to IDT entry.
  @param[in, out] PageFaultContext  Pointer to page fault context.

**/
VOID
HookPageFaultHandler (
  IN OUT IA32_IDT_GATE_DESCRIPTOR  *IdtEntry,
  IN OUT PAGE_FAULT_CONTEXT        *PageFaultContext
  )
{
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;
  UINTN   PageFaultHandlerHookAddress;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8)RegEax;
  } else {
    PhysicalAddressBits = 36;
  }

  PageFaultContext->PhyMask  = LShiftU64 (1, PhysicalAddressBits) - 1;
  PageFaultContext->PhyMask &= (1ull << 48) - SIZE_4KB;

  //
  // Set Page Fault entry to catch >4G access
  //
  PageFaultHandlerHookAddress       = (UINTN)PageFaultHandlerHook;
  PageFaultContext->OriginalHandler = (VOID *)(UINTN)(LShiftU64 (IdtEntry->Bits.OffsetUpper, 32) + IdtEntry->Bits.OffsetLow + (IdtEntry->Bits.OffsetHigh << 16));
  IdtEntry->Bits.OffsetLow          = (UINT16)PageFaultHandlerHookAddress;
  IdtEntry->Bits.Selector           = (UINT16)AsmReadCs ();
  IdtEntry->Bits.Reserved_0         = 0;
  IdtEntry->Bits.GateType           = IA32_IDT_GATE_TYPE_INTERRUPT_32;
  IdtEntry->Bits.OffsetHigh         = (UINT16)(PageFaultHandlerHookAddress >> 16);
  IdtEntry->Bits.OffsetUpper        = (UINT32)(PageFaultHandlerHookAddress >> 32);
  IdtEntry->Bits.Reserved_1         = 0;

  if (PageFaultContext->Page1GSupport) {
    PageFaultContext->PageFaultBuffer = (UINTN)(AsmReadCr3 () & PageFaultContext->PhyMask) + EFI_PAGES_TO_SIZE (2);
  } else {
    PageFaultContext->PageFaultBuffer = (UINTN)(AsmReadCr3 () & PageFaultContext->PhyMask) + EFI_PAGES_TO_SIZE (6);
  }

  PageFaultContext->PageFaultIndex = 0;
  ZeroMem (PageFaultContext->PageFaultUplink, sizeof (PageFaultContext->PageFaultUplink));
}

/**
  Acquire page for page fault.

  @param[in, out] PageFaultContext  Pointer to page fault context.
  @param[in, out] Uplink            Pointer to up page table entry.

**/
VOID
AcquirePage (
  IN OUT PAGE_FAULT_CONTEXT  *PageFaultContext,
  IN OUT UINT64              *Uplink
  )
{
  UINTN   Address;
  UINT64  AddressEncMask;

  Address = PageFaultContext->PageFaultBuffer + EFI_PAGES_TO_SIZE (PageFaultContext->PageFaultIndex);
  ZeroMem ((VOID *)Address, EFI_PAGES_TO_SIZE (1));

  AddressEncMask = PageFaultContext->AddressEncMask;

  //
  // Cut the previous uplink if it exists and wasn't overwritten.
  //
  if ((PageFaultContext->PageFaultUplink[PageFaultContext->PageFaultIndex] != NULL) &&
      ((*PageFaultContext->PageFaultUplink[PageFaultContext->PageFaultIndex] & ~AddressEncMask & PageFaultContext->PhyMask) == Address))
  {
    *PageFaultContext->PageFaultUplink[PageFaultContext->PageFaultIndex] = 0;
  }

  //
  // Link & Record the current uplink.
  //
  *Uplink                                                             = Address | AddressEncMask | IA32_PG_P | IA32_PG_RW;
  PageFaultContext->PageFaultUplink[PageFaultContext->PageFaultIndex] = Uplink;

  PageFaultContext->PageFaultIndex = (PageFaultContext->PageFaultIndex + 1) % EXTRA_PAGE_TABLE_PAGES;
}

/**
  The page fault handler that on-demand read >4G memory/MMIO.

  @retval NULL              The page fault is correctly handled.
  @retval OriginalHandler   The page fault is not handled and is passed through to original handler.

**/
VOID *
EFIAPI
PageFaultHandler (
  VOID
  )
{
  IA32_DESCRIPTOR     Idtr;
  PAGE_FAULT_CONTEXT  *PageFaultContext;
  UINT64              PhyMask;
  UINT64              *PageTable;
  UINT64              PFAddress;
  UINTN               PTIndex;
  UINT64              AddressEncMask;

  //
  // Get the IDT Descriptor.
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *)&Idtr);
  //
  // Then get page fault context by IDT Descriptor.
  //
  PageFaultContext = (PAGE_FAULT_CONTEXT *)(UINTN)(Idtr.Base - sizeof (PAGE_FAULT_CONTEXT));
  PhyMask          = PageFaultContext->PhyMask;
  AddressEncMask   = PageFaultContext->AddressEncMask;

  PFAddress = AsmReadCr2 ();
  DEBUG ((DEBUG_ERROR, "CapsuleX64 - PageFaultHandler: Cr2 - %lx\n", PFAddress));

  if (PFAddress >= PhyMask + SIZE_4KB) {
    return PageFaultContext->OriginalHandler;
  }

  PFAddress &= PhyMask;

  PageTable = (UINT64 *)(UINTN)(AsmReadCr3 () & PhyMask);

  PTIndex = BitFieldRead64 (PFAddress, 39, 47);
  // PML4E
  if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
    AcquirePage (PageFaultContext, &PageTable[PTIndex]);
  }

  PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~AddressEncMask & PhyMask);
  PTIndex   = BitFieldRead64 (PFAddress, 30, 38);
  // PDPTE
  if (PageFaultContext->Page1GSupport) {
    PageTable[PTIndex] = ((PFAddress | AddressEncMask) & ~((1ull << 30) - 1)) | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  } else {
    if ((PageTable[PTIndex] & IA32_PG_P) == 0) {
      AcquirePage (PageFaultContext, &PageTable[PTIndex]);
    }

    PageTable = (UINT64 *)(UINTN)(PageTable[PTIndex] & ~AddressEncMask & PhyMask);
    PTIndex   = BitFieldRead64 (PFAddress, 21, 29);
    // PD
    PageTable[PTIndex] = ((PFAddress | AddressEncMask) & ~((1ull << 21) - 1)) | IA32_PG_P | IA32_PG_RW | IA32_PG_PS;
  }

  return NULL;
}

/**
  The X64 entrypoint is used to process capsule in long mode then
  return to 32-bit protected mode.

  @param  EntrypointContext   Pointer to the context of long mode.
  @param  ReturnContext       Pointer to the context of 32-bit protected mode.

  @retval This function should never return actually.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  SWITCH_32_TO_64_CONTEXT  *EntrypointContext,
  SWITCH_64_TO_32_CONTEXT  *ReturnContext
  )
{
  EFI_STATUS                Status;
  IA32_DESCRIPTOR           Ia32Idtr;
  IA32_DESCRIPTOR           X64Idtr;
  PAGE_FAULT_IDT_TABLE      PageFaultIdtTable;
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;

  //
  // Save the IA32 IDT Descriptor
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *)&Ia32Idtr);

  //
  // Setup X64 IDT table
  //
  ZeroMem (PageFaultIdtTable.IdtEntryTable, sizeof (IA32_IDT_GATE_DESCRIPTOR) * EXCEPTION_VECTOR_NUMBER);
  X64Idtr.Base  = (UINTN)PageFaultIdtTable.IdtEntryTable;
  X64Idtr.Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * EXCEPTION_VECTOR_NUMBER - 1);
  AsmWriteIdtr ((IA32_DESCRIPTOR *)&X64Idtr);

  //
  // Setup the default CPU exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Hook page fault handler to handle >4G request.
  //
  PageFaultIdtTable.PageFaultContext.Page1GSupport  = EntrypointContext->Page1GSupport;
  PageFaultIdtTable.PageFaultContext.AddressEncMask = EntrypointContext->AddressEncMask;
  IdtEntry                                          = (IA32_IDT_GATE_DESCRIPTOR *)(X64Idtr.Base + (14 * sizeof (IA32_IDT_GATE_DESCRIPTOR)));
  HookPageFaultHandler (IdtEntry, &(PageFaultIdtTable.PageFaultContext));

  //
  // Initialize Debug Agent to support source level debug
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64, (VOID *)&Ia32Idtr, NULL);

  //
  // Call CapsuleDataCoalesce to process capsule.
  //
  Status = CapsuleDataCoalesce (
             NULL,
             (EFI_PHYSICAL_ADDRESS *)(UINTN)EntrypointContext->BlockListAddr,
             (MEMORY_RESOURCE_DESCRIPTOR *)(UINTN)EntrypointContext->MemoryResource,
             (VOID **)(UINTN)EntrypointContext->MemoryBase64Ptr,
             (UINTN *)(UINTN)EntrypointContext->MemorySize64Ptr
             );

  ReturnContext->ReturnStatus = Status;

  DEBUG ((
    DEBUG_INFO,
    "%a() Stack Base: 0x%lx, Stack Size: 0x%lx\n",
    __func__,
    EntrypointContext->StackBufferBase,
    EntrypointContext->StackBufferLength
    ));

  //
  // Disable interrupt of Debug timer, since the new IDT table cannot work in long mode
  //
  SaveAndSetDebugTimerInterrupt (FALSE);
  //
  // Restore IA32 IDT table
  //
  AsmWriteIdtr ((IA32_DESCRIPTOR *)&Ia32Idtr);

  //
  // Finish to coalesce capsule, and return to 32-bit mode.
  //
  AsmDisablePaging64 (
    ReturnContext->ReturnCs,
    (UINT32)ReturnContext->ReturnEntryPoint,
    (UINT32)(UINTN)EntrypointContext,
    (UINT32)(UINTN)ReturnContext,
    (UINT32)(EntrypointContext->StackBufferBase + EntrypointContext->StackBufferLength)
    );

  //
  // Should never be here.
  //
  ASSERT (FALSE);
  return EFI_SUCCESS;
}
