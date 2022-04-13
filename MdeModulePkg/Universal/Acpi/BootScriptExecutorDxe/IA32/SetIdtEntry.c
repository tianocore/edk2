/** @file
  Set a IDT entry for debug purpose

  Set a IDT entry for interrupt vector 3 for debug purpose for IA32 platform

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "ScriptExecute.h"

/**
  Set a IDT entry for interrupt vector 3 for debug purpose.

  @param  AcpiS3Context  a pointer to a structure of ACPI_S3_CONTEXT

**/
VOID
SetIdtEntry (
  IN ACPI_S3_CONTEXT  *AcpiS3Context
  )
{
  IA32_IDT_GATE_DESCRIPTOR  *IdtEntry;
  IA32_DESCRIPTOR           *IdtDescriptor;
  UINTN                     S3DebugBuffer;
  EFI_STATUS                Status;

  //
  // Restore IDT for debug
  //
  IdtDescriptor = (IA32_DESCRIPTOR *)(UINTN)(AcpiS3Context->IdtrProfile);
  AsmWriteIdtr (IdtDescriptor);

  //
  // Setup the default CPU exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE_BEGIN ();
  //
  // Update IDT entry INT3 if the instruction is valid in it
  //
  S3DebugBuffer = (UINTN)(AcpiS3Context->S3DebugBufferAddress);
  if (*(UINTN *)S3DebugBuffer != (UINTN)-1) {
    IdtEntry                  = (IA32_IDT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (IA32_IDT_GATE_DESCRIPTOR)));
    IdtEntry->Bits.OffsetLow  = (UINT16)S3DebugBuffer;
    IdtEntry->Bits.Selector   = (UINT16)AsmReadCs ();
    IdtEntry->Bits.Reserved_0 = 0;
    IdtEntry->Bits.GateType   = IA32_IDT_GATE_TYPE_INTERRUPT_32;
    IdtEntry->Bits.OffsetHigh = (UINT16)(S3DebugBuffer >> 16);
  }

  DEBUG_CODE_END ();
}
