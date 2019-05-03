/** @file
  This library defines routines to dump CPU structure information to debug output.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Dumps the Interrupt Descriptor Table (IDT) to debug output.

**/
VOID
EFIAPI
DumpIdt (
  VOID
  )
{
  DEBUG_CODE_BEGIN ();
  IA32_IDT_GATE_DESCRIPTOR          *IdtTable;
  IA32_DESCRIPTOR                   Idtr;
  UINTN                             Index;
  UINTN                             IdtEntryCount;
  UINTN                             IdtTableSize;

  AsmReadIdtr ((IA32_DESCRIPTOR *) &Idtr);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *) Idtr.Base;
  IdtTableSize = Idtr.Limit + 1;
  IdtEntryCount = IdtTableSize / sizeof (IA32_IDT_GATE_DESCRIPTOR);

  DEBUG ((DEBUG_INFO, "PeiCoreDbg: Dumping IDT:\n"));
  DEBUG ((DEBUG_INFO, "PeiCoreDbg: IDT at 0x%x. Entries: %d. Size: 0x%x\n\n", (UINTN) IdtTable, IdtEntryCount, IdtTableSize));
  for (Index = 0; Index < IdtEntryCount; Index++) {
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:   Entry[%03d]\n", Index));
    DEBUG ((
      DEBUG_INFO,
      "PeiCoreDbg:     Offset      = 0x%x\n",
      ((IdtTable[Index].Bits.OffsetHigh << 0x10) | IdtTable[Index].Bits.OffsetLow)
      ));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:     Selector    = 0x%x\n", IdtTable[Index].Bits.Selector));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:     Gate Type   = "));
    switch (IdtTable[Index].Bits.GateType) {
      case IA32_IDT_GATE_TYPE_TASK:
        DEBUG ((DEBUG_INFO, "%a", "Task"));
        break;
      case IA32_IDT_GATE_TYPE_INTERRUPT_16:
        DEBUG ((DEBUG_INFO, "%a", "Interrupt (16-bit)"));
        break;
      case IA32_IDT_GATE_TYPE_TRAP_16:
        DEBUG ((DEBUG_INFO, "%a", "Trap (16-bit"));
        break;
      case IA32_IDT_GATE_TYPE_INTERRUPT_32:
        DEBUG ((DEBUG_INFO, "%a", "Interrupt (32-bit)"));
        break;
      case IA32_IDT_GATE_TYPE_TRAP_32:
        DEBUG ((DEBUG_INFO, "%a", "Trap (32-bit)"));
        break;
      default:
        DEBUG ((DEBUG_INFO, "%a", "Invalid"));
    }
    DEBUG ((DEBUG_INFO, " (0x%x)\n", IdtTable[Index].Bits.GateType));
  }
  DEBUG_CODE_END ();
}

/**
  Dumps the Global Descriptor Table (GDT) to debug output.

**/
VOID
EFIAPI
DumpGdt (
  VOID
  )
{
  DEBUG_CODE_BEGIN ();
  IA32_SEGMENT_DESCRIPTOR   *GdtTable;
  IA32_DESCRIPTOR           Gdtr;
  UINTN                     GdtTableSize;
  UINTN                     GdtEntryCount;
  UINTN                     Index;

  AsmReadGdtr ((IA32_DESCRIPTOR *) &Gdtr);
  GdtTable = (IA32_SEGMENT_DESCRIPTOR *) Gdtr.Base;
  GdtTableSize = Gdtr.Limit + 1;
  GdtEntryCount = GdtTableSize / sizeof (IA32_SEGMENT_DESCRIPTOR);

  DEBUG ((DEBUG_INFO, "PeiCoreDbg: Dumping GDT:\n"));
  DEBUG ((DEBUG_INFO, "PeiCoreDbg: GDT at 0x%x. Entries: %d. Size: 0x%x\n\n", (UINTN) GdtTable, GdtEntryCount, GdtTableSize));
  for (Index = 0; Index < GdtEntryCount; Index++) {
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:   Entry[%04d]\n", Index));
    DEBUG ((
      DEBUG_INFO,
      "PeiCoreDbg:     Base = 0x%x\n",
      ((GdtTable[Index].Bits.BaseHigh << 0x18) | (GdtTable[Index].Bits.BaseMid << 0x10) | GdtTable[Index].Bits.BaseLow)
      ));
    DEBUG ((
      DEBUG_INFO,
      "PeiCoreDbg:     Limit  = 0x%x\n",
      ((GdtTable[Index].Bits.LimitHigh << 0x10) | GdtTable[Index].Bits.LimitLow)
      ));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:     Access Bytes:\n"));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:       Type: 0x%x\n", GdtTable[Index].Bits.Type));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         Accessed             : 0x%x\n", (GdtTable[Index].Bits.Type & 0x1) != 0));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         RW                   : 0x%x\n", (GdtTable[Index].Bits.Type & 0x2) != 0));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         Direction/Conforming : 0x%x\n", (GdtTable[Index].Bits.Type & 0x4) != 0));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         Executable           : 0x%x\n", (GdtTable[Index].Bits.Type & 0x8) != 0));

    DEBUG ((DEBUG_INFO, "PeiCoreDbg:       Descriptor Type (S)    : 0x%x\n", GdtTable[Index].Bits.S));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:       Privilege (DPL)        : 0x%x\n", GdtTable[Index].Bits.DPL));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:       Present (P)            : 0x%x\n", GdtTable[Index].Bits.P));

    DEBUG ((DEBUG_INFO, "PeiCoreDbg:       Flags:\n"));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         AVL                  : 0x%x\n", GdtTable[Index].Bits.AVL));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         L                    : 0x%x\n", GdtTable[Index].Bits.L));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         DB                   : 0x%x\n", GdtTable[Index].Bits.DB));
    DEBUG ((DEBUG_INFO, "PeiCoreDbg:         G                    : 0x%x\n", GdtTable[Index].Bits.G));
  }
  DEBUG_CODE_END ();
}
