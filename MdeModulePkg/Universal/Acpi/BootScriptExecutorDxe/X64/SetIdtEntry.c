/** @file
  Set a IDT entry for debug purpose

  Set a IDT entry for interrupt vector 3 for debug purpose for x64 platform

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>

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
  AsmWriteIdtr (IdtDescriptor);

  //
  // Setup the default CPU exception handlers
  //
  SetupCpuExceptionHandlers ();

  //
  // Update IDT entry INT3
  //
  IdtEntry = (INTERRUPT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (INTERRUPT_GATE_DESCRIPTOR)));
  S3DebugBuffer = (UINTN) (AcpiS3Context->S3DebugBufferAddress);

  IdtEntry->Offset15To0     = (UINT16)S3DebugBuffer;
  IdtEntry->SegmentSelector = (UINT16)AsmReadCs ();;
  IdtEntry->Attributes      = (UINT16)INTERRUPT_GATE_ATTRIBUTE;
  IdtEntry->Offset31To16    = (UINT16)(S3DebugBuffer >> 16);
  IdtEntry->Offset63To32    = (UINT32)(S3DebugBuffer >> 32);
  IdtEntry->Reserved        = 0;

}

