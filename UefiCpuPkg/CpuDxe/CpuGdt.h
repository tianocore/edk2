/** @file
  C based implementation of IA32 interrupt handling only
  requiring a minimal assembly interrupt entry point.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_GDT_H_
#define _CPU_GDT_H_

//
// Local structure definitions
//

#pragma pack (1)

//
// Global Descriptor Entry structures
//

typedef struct _GDT_ENTRY {
  UINT16    Limit15_0;
  UINT16    Base15_0;
  UINT8     Base23_16;
  UINT8     Type;
  UINT8     Limit19_16_and_flags;
  UINT8     Base31_24;
} GDT_ENTRY;

typedef
  struct _GDT_ENTRIES {
  GDT_ENTRY    Null;
  GDT_ENTRY    Linear;
  GDT_ENTRY    LinearCode;
  GDT_ENTRY    SysData;
  GDT_ENTRY    SysCode;
  GDT_ENTRY    SysCode16;
  GDT_ENTRY    LinearData64;
  GDT_ENTRY    LinearCode64;
  GDT_ENTRY    Spare5;
} GDT_ENTRIES;

#pragma pack ()

#define NULL_SEL           OFFSET_OF (GDT_ENTRIES, Null)
#define LINEAR_SEL         OFFSET_OF (GDT_ENTRIES, Linear)
#define LINEAR_CODE_SEL    OFFSET_OF (GDT_ENTRIES, LinearCode)
#define SYS_DATA_SEL       OFFSET_OF (GDT_ENTRIES, SysData)
#define SYS_CODE_SEL       OFFSET_OF (GDT_ENTRIES, SysCode)
#define SYS_CODE16_SEL     OFFSET_OF (GDT_ENTRIES, SysCode16)
#define LINEAR_DATA64_SEL  OFFSET_OF (GDT_ENTRIES, LinearData64)
#define LINEAR_CODE64_SEL  OFFSET_OF (GDT_ENTRIES, LinearCode64)
#define SPARE5_SEL         OFFSET_OF (GDT_ENTRIES, Spare5)

#if defined (MDE_CPU_IA32)
#define CPU_CODE_SEL  LINEAR_CODE_SEL
#define CPU_DATA_SEL  LINEAR_SEL
#elif defined (MDE_CPU_X64)
#define CPU_CODE_SEL  LINEAR_CODE64_SEL
#define CPU_DATA_SEL  LINEAR_DATA64_SEL
#else
  #error CPU type not supported for CPU GDT initialization!
#endif

#endif // _CPU_GDT_H_
