/** @file
Set a IDT entry for debug purpose

Set a IDT entry for interrupt vector 3 for debug purpose for IA32 platform

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "ScriptExecute.h"
//
// INTERRUPT_GATE_DESCRIPTOR and SetIdtEntry () are used to setup IDT to do debug
//

#pragma pack(1)

typedef struct {
  UINT16  OffsetLow;
  UINT16  SegmentSelector;
  UINT16  Attributes;
  UINT16  OffsetHigh;
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
  IdtEntry = (INTERRUPT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (INTERRUPT_GATE_DESCRIPTOR)));
  S3DebugBuffer = (UINTN) (AcpiS3Context->S3DebugBufferAddress);

  IdtEntry->OffsetLow       = (UINT16)S3DebugBuffer;
  IdtEntry->SegmentSelector = (UINT16)AsmReadCs ();
  IdtEntry->Attributes      = (UINT16)INTERRUPT_GATE_ATTRIBUTE;
  IdtEntry->OffsetHigh      = (UINT16)(S3DebugBuffer >> 16);

  AsmWriteIdtr (IdtDescriptor);
}

