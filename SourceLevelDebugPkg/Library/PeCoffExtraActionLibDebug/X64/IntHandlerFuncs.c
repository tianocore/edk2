/** @file
  X64 arch function to access IDT vector.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PeCoffExtraActionLib.h>

/**
  Read IDT entry to check if IDT entries are setup by Debug Agent.

  @param[in]  IdtDescriptor      Pointer to IDT Descriptor.
  @param[in]  InterruptType      Interrupt type.

  @retval  TRUE     IDT entries were setup by Debug Agent.
  @retval  FALSE    IDT entries were not setuo by Debug Agent.

**/
BOOLEAN
CheckDebugAgentHandler (
  IN  IA32_DESCRIPTOR  *IdtDescriptor,
  IN  UINTN            InterruptType
  )
{
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;
  UINTN                     InterruptHandler;

  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *)IdtDescriptor->Base;
  if (IdtEntry == NULL) {
    return FALSE;
  }

  InterruptHandler = IdtEntry[InterruptType].Bits.OffsetLow +
                     (((UINTN)IdtEntry[InterruptType].Bits.OffsetHigh) << 16) +
                     (((UINTN)IdtEntry[InterruptType].Bits.OffsetUpper) << 32);
  if ((InterruptHandler >= sizeof (UINT32)) &&  (*(UINT32 *)(InterruptHandler - sizeof (UINT32)) == AGENT_HANDLER_SIGNATURE)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Save IDT entry for INT1 and update it.

  @param[in]  IdtDescriptor      Pointer to IDT Descriptor.
  @param[out] SavedIdtEntry      Original IDT entry returned.

**/
VOID
SaveAndUpdateIdtEntry1 (
  IN  IA32_DESCRIPTOR           *IdtDescriptor,
  OUT IA32_IDT_GATE_DESCRIPTOR  *SavedIdtEntry
  )
{
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;
  UINT16                    CodeSegment;
  UINTN                     InterruptHandler;

  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *)IdtDescriptor->Base;
  CopyMem (SavedIdtEntry, &IdtEntry[1], sizeof (IA32_IDT_GATE_DESCRIPTOR));

  //
  // Use current CS as the segment selector of interrupt gate in IDT
  //
  CodeSegment = AsmReadCs ();

  InterruptHandler             = (UINTN)&AsmInterruptHandle;
  IdtEntry[1].Bits.OffsetLow   = (UINT16)(UINTN)InterruptHandler;
  IdtEntry[1].Bits.OffsetHigh  = (UINT16)((UINTN)InterruptHandler >> 16);
  IdtEntry[1].Bits.OffsetUpper = (UINT32)((UINTN)InterruptHandler >> 32);
  IdtEntry[1].Bits.Selector    = CodeSegment;
  IdtEntry[1].Bits.GateType    = IA32_IDT_GATE_TYPE_INTERRUPT_32;
}

/**
  Restore IDT entry for INT1.

  @param[in]  IdtDescriptor      Pointer to IDT Descriptor.
  @param[in]  RestoredIdtEntry   IDT entry to be restored.

**/
VOID
RestoreIdtEntry1 (
  IN  IA32_DESCRIPTOR           *IdtDescriptor,
  IN  IA32_IDT_GATE_DESCRIPTOR  *RestoredIdtEntry
  )
{
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;

  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *)IdtDescriptor->Base;
  CopyMem (&IdtEntry[1], RestoredIdtEntry, sizeof (IA32_IDT_GATE_DESCRIPTOR));
}
