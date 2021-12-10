/** @file
  Initialize GDT for Linux.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LoadLinuxLib.h"

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
  GDT_ENTRY    Null2;
  GDT_ENTRY    Linear;
  GDT_ENTRY    LinearCode;
  GDT_ENTRY    TaskSegment;
  GDT_ENTRY    Spare4;
  GDT_ENTRY    Spare5;
} GDT_ENTRIES;

#pragma pack ()

STATIC GDT_ENTRIES  *mGdt = NULL;

//
// Global descriptor table (GDT) Template
//
STATIC GDT_ENTRIES  GdtTemplate = {
  //
  // Null
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
  // Null2
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
  // Linear
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
  // LinearCode
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
  // TaskSegment
  //
  {
    0x0,            // limit 0
    0x0,            // base 0
    0x0,
    0x089,          // ?
    0x080,          // ?
    0x0,
  },
  //
  // Spare4
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
  // Spare5
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
InitLinuxDescriptorTables (
  VOID
  )
{
  //
  // Allocate Runtime Data for the GDT
  //
  mGdt = AllocateRuntimePool (sizeof (GdtTemplate) + 8);
  ASSERT (mGdt != NULL);
  mGdt = ALIGN_POINTER (mGdt, 8);

  //
  // Initialize all GDT entries
  //
  CopyMem (mGdt, &GdtTemplate, sizeof (GdtTemplate));
}

/**
  Initialize Global Descriptor Table.

**/
VOID
SetLinuxDescriptorTables (
  VOID
  )
{
  IA32_DESCRIPTOR  GdtPtr;
  IA32_DESCRIPTOR  IdtPtr;

  //
  // Write GDT register
  //
  GdtPtr.Base  = (UINT32)(UINTN)(VOID *)mGdt;
  GdtPtr.Limit = (UINT16)(sizeof (GdtTemplate) - 1);
  AsmWriteGdtr (&GdtPtr);

  IdtPtr.Base  = (UINT32)0;
  IdtPtr.Limit = (UINT16)0;
  AsmWriteIdtr (&IdtPtr);
}
