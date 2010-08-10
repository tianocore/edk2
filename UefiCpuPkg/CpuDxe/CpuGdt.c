/** @file
  C based implemention of IA32 interrupt handling only
  requiring a minimal assembly interrupt entry point.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"


//
// Local structure definitions
//

#pragma pack (1)

//
// Global Descriptor Entry structures
//

typedef struct _GDT_ENTRY {
  UINT16 Limit15_0;
  UINT16 Base15_0;
  UINT8  Base23_16;
  UINT8  Type;
  UINT8  Limit19_16_and_flags;
  UINT8  Base31_24;
} GDT_ENTRY;

typedef
struct _GDT_ENTRIES {
  GDT_ENTRY Null;
  GDT_ENTRY Linear;
  GDT_ENTRY LinearCode;
  GDT_ENTRY SysData;
  GDT_ENTRY SysCode;
  GDT_ENTRY LinearCode64;
  GDT_ENTRY Spare4;
  GDT_ENTRY Spare5;
} GDT_ENTRIES;

#define NULL_SEL          OFFSET_OF (GDT_ENTRIES, Null)
#define LINEAR_SEL        OFFSET_OF (GDT_ENTRIES, Linear)
#define LINEAR_CODE_SEL   OFFSET_OF (GDT_ENTRIES, LinearCode)
#define SYS_DATA_SEL      OFFSET_OF (GDT_ENTRIES, SysData)
#define SYS_CODE_SEL      OFFSET_OF (GDT_ENTRIES, SysCode)
#define LINEAR_CODE64_SEL OFFSET_OF (GDT_ENTRIES, LinearCode64)
#define SPARE4_SEL        OFFSET_OF (GDT_ENTRIES, Spare4)
#define SPARE5_SEL        OFFSET_OF (GDT_ENTRIES, Spare5)

#if defined (MDE_CPU_IA32)
#define CPU_CODE_SEL LINEAR_CODE_SEL
#define CPU_DATA_SEL LINEAR_SEL
#elif defined (MDE_CPU_X64)
#define CPU_CODE_SEL LINEAR_CODE64_SEL
#define CPU_DATA_SEL LINEAR_SEL
#else
#error CPU type not supported for CPU GDT initialization!
#endif

//
// Global descriptor table (GDT) Template
//
STATIC GDT_ENTRIES GdtTemplate = {
  //
  // NULL_SEL
  //
  {
    0x0,            // limit 15:0
    0x0,            // base 15:0
    0x0,            // base 23:16
    0x0,            // type
    0x0,            // limit 19:16, flags
    0x0,            // base 31:24
  },
  //
  // LINEAR_SEL
  //
  {
    0x0FFFF,        // limit 0xFFFFF
    0x0,            // base 0
    0x0,
    0x092,          // present, ring 0, data, expand-up, writable
    0x0CF,          // page-granular, 32-bit
    0x0,
  },
  //
  // LINEAR_CODE_SEL
  //
  {
    0x0FFFF,        // limit 0xFFFFF
    0x0,            // base 0
    0x0,
    0x09A,          // present, ring 0, data, expand-up, writable
    0x0CF,          // page-granular, 32-bit
    0x0,
  },
  //
  // SYS_DATA_SEL
  //
  {
    0x0FFFF,        // limit 0xFFFFF
    0x0,            // base 0
    0x0,
    0x092,          // present, ring 0, data, expand-up, writable
    0x0CF,          // page-granular, 32-bit
    0x0,
  },
  //
  // SYS_CODE_SEL
  //
  {
    0x0FFFF,        // limit 0xFFFFF
    0x0,            // base 0
    0x0,
    0x09A,          // present, ring 0, data, expand-up, writable
    0x0CF,          // page-granular, 32-bit
    0x0,
  },
  //
  // LINEAR_CODE64_SEL
  //
  {
    0x0FFFF,        // limit 0xFFFFF
    0x0,            // base 0
    0x0,
    0x09B,          // present, ring 0, code, expand-up, writable
    0x0AF,          // LimitHigh (CS.L=1, CS.D=0)
    0x0,            // base (high)
  },
  //
  // SPARE4_SEL
  //
  {
    0x0,            // limit 0
    0x0,            // base 0
    0x0,
    0x0,            // present, ring 0, data, expand-up, writable
    0x0,            // page-granular, 32-bit
    0x0,
  },
  //
  // SPARE5_SEL
  //
  {
    0x0,            // limit 0
    0x0,            // base 0
    0x0,
    0x0,            // present, ring 0, data, expand-up, writable
    0x0,            // page-granular, 32-bit
    0x0,
  },
};

/**
  Initialize Global Descriptor Table.

**/
VOID
InitGlobalDescriptorTable (
  VOID
  )
{
  GDT_ENTRIES *gdt;
  IA32_DESCRIPTOR gdtPtr;

  //
  // Allocate Runtime Data for the GDT
  //
  gdt = AllocateRuntimePool (sizeof (GdtTemplate) + 8);
  ASSERT (gdt != NULL);
  gdt = ALIGN_POINTER (gdt, 8);

  //
  // Initialize all GDT entries
  //
  CopyMem (gdt, &GdtTemplate, sizeof (GdtTemplate));

  //
  // Write GDT register
  //
  gdtPtr.Base = (UINT32)(UINTN)(VOID*) gdt;
  gdtPtr.Limit = (UINT16) (sizeof (GdtTemplate) - 1);
  AsmWriteGdtr (&gdtPtr);

  //
  // Update selector (segment) registers base on new GDT
  //
  SetCodeSelector ((UINT16)CPU_CODE_SEL);
  SetDataSelectors ((UINT16)CPU_DATA_SEL);
}

