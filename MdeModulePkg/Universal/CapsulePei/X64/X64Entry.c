/** @file
  The X64 entrypoint is used to process capsule in long mode.

Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugAgentLib.h>
#include "CommonHeader.h"

#define EXCEPTION_VECTOR_NUMBER     0x22

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
  SWITCH_32_TO_64_CONTEXT       *EntrypointContext,
  SWITCH_64_TO_32_CONTEXT       *ReturnContext
)
{
  EFI_STATUS                    Status;
  IA32_DESCRIPTOR               Ia32Idtr;
  IA32_DESCRIPTOR               X64Idtr;
  IA32_IDT_GATE_DESCRIPTOR      IdtEntryTable[EXCEPTION_VECTOR_NUMBER];

  //
  // Save the IA32 IDT Descriptor
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &Ia32Idtr); 

  //
  // Setup X64 IDT table
  //
  ZeroMem (IdtEntryTable, sizeof (IA32_IDT_GATE_DESCRIPTOR) * EXCEPTION_VECTOR_NUMBER);
  X64Idtr.Base = (UINTN) IdtEntryTable;
  X64Idtr.Limit = (UINT16) (sizeof (IA32_IDT_GATE_DESCRIPTOR) * EXCEPTION_VECTOR_NUMBER - 1);
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &X64Idtr);  

  //
  // Setup the default CPU exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);
  
  //
  // Initialize Debug Agent to support source level debug
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64, (VOID *) &Ia32Idtr, NULL);

  //
  // Call CapsuleDataCoalesce to process capsule.
  //
  Status = CapsuleDataCoalesce (
             NULL,
             (EFI_PHYSICAL_ADDRESS *) (UINTN) EntrypointContext->BlockListAddr,
             (VOID **) (UINTN) EntrypointContext->MemoryBase64Ptr,
             (UINTN *) (UINTN) EntrypointContext->MemorySize64Ptr
             );
  
  ReturnContext->ReturnStatus = Status;

  //
  // Disable interrupt of Debug timer, since the new IDT table cannot work in long mode
  //
  SaveAndSetDebugTimerInterrupt (FALSE);
  //
  // Restore IA32 IDT table
  //
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &Ia32Idtr);  
  
  //
  // Finish to coalesce capsule, and return to 32-bit mode.
  //
  AsmDisablePaging64 (
    ReturnContext->ReturnCs,
    (UINT32) ReturnContext->ReturnEntryPoint,
    (UINT32) (UINTN) EntrypointContext,
    (UINT32) (UINTN) ReturnContext,
    (UINT32) (EntrypointContext->StackBufferBase + EntrypointContext->StackBufferLength)
    );  
  
  //
  // Should never be here.
  //
  ASSERT (FALSE);
  return EFI_SUCCESS;
}