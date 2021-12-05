/** @file
  IA32/x64 generic functions to support Debug Support protocol.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugSupport.h"

//
// This the global main table to keep track of the interrupts
//
IDT_ENTRY  *IdtEntryTable = NULL;

/**
  Read IDT Gate Descriptor from IDT Table.

  @param  Vector            Specifies vector number.
  @param  IdtGateDescriptor Pointer to IDT Gate Descriptor read from IDT Table.

**/
VOID
ReadIdtGateDescriptor (
  IN  EFI_EXCEPTION_TYPE        Vector,
  OUT IA32_IDT_GATE_DESCRIPTOR  *IdtGateDescriptor
  )
{
  IA32_DESCRIPTOR           IdtrValue;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;

  AsmReadIdtr (&IdtrValue);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *)IdtrValue.Base;

  CopyMem ((VOID *)IdtGateDescriptor, (VOID *)&(IdtTable)[Vector], sizeof (IA32_IDT_GATE_DESCRIPTOR));
}

/**
  Write IDT Gate Descriptor into IDT Table.

  @param  Vector            Specifies vector number.
  @param  IdtGateDescriptor Pointer to IDT Gate Descriptor written into IDT Table.

**/
VOID
WriteIdtGateDescriptor (
  EFI_EXCEPTION_TYPE        Vector,
  IA32_IDT_GATE_DESCRIPTOR  *IdtGateDescriptor
  )
{
  IA32_DESCRIPTOR           IdtrValue;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;

  AsmReadIdtr (&IdtrValue);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *)IdtrValue.Base;

  CopyMem ((VOID *)&(IdtTable)[Vector], (VOID *)IdtGateDescriptor, sizeof (IA32_IDT_GATE_DESCRIPTOR));
}

/**
  Creates a nes entry stub.  Then saves the current IDT entry and replaces it
  with an interrupt gate for the new entry point.  The IdtEntryTable is updated
  with the new registered function.

  This code executes in boot services context.  The stub entry executes in interrupt
  context.

  @param  ExceptionType      Specifies which vector to hook.
  @param  NewCallback        A pointer to the new function to be registered.

**/
VOID
HookEntry (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN CALLBACK_FUNC       NewCallback
  )
{
  BOOLEAN  OldIntFlagState;

  CreateEntryStub (ExceptionType, (VOID **)&IdtEntryTable[ExceptionType].StubEntry);

  //
  // Disables CPU interrupts and returns the previous interrupt state
  //
  OldIntFlagState = SaveAndDisableInterrupts ();

  //
  // gets IDT Gate descriptor by index
  //
  ReadIdtGateDescriptor (ExceptionType, &(IdtEntryTable[ExceptionType].OrigDesc));
  //
  // stores orignal interrupt handle
  //
  IdtEntryTable[ExceptionType].OrigVector = (DEBUG_PROC)GetInterruptHandleFromIdt (&(IdtEntryTable[ExceptionType].OrigDesc));

  //
  // encodes new IDT Gate descriptor by stub entry
  //
  Vect2Desc (&IdtEntryTable[ExceptionType].NewDesc, IdtEntryTable[ExceptionType].StubEntry);
  //
  // stores NewCallback
  //
  IdtEntryTable[ExceptionType].RegisteredCallback = NewCallback;

  //
  // writes back new IDT Gate descriptor
  //
  WriteIdtGateDescriptor (ExceptionType, &(IdtEntryTable[ExceptionType].NewDesc));

  //
  // restore interrupt state
  //
  SetInterruptState (OldIntFlagState);

  return;
}

/**
  Undoes HookEntry. This code executes in boot services context.

  @param  ExceptionType   Specifies which entry to unhook

**/
VOID
UnhookEntry (
  IN EFI_EXCEPTION_TYPE  ExceptionType
  )
{
  BOOLEAN  OldIntFlagState;

  //
  // Disables CPU interrupts and returns the previous interrupt state
  //
  OldIntFlagState = SaveAndDisableInterrupts ();

  //
  // restore the default IDT Date Descriptor
  //
  WriteIdtGateDescriptor (ExceptionType, &(IdtEntryTable[ExceptionType].OrigDesc));

  //
  // restore interrupt state
  //
  SetInterruptState (OldIntFlagState);

  return;
}

/**
  Returns the maximum value that may be used for the ProcessorIndex parameter in
  RegisterPeriodicCallback() and RegisterExceptionCallback().

  Hard coded to support only 1 processor for now.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  MaxProcessorIndex     Pointer to a caller-allocated UINTN in which the maximum supported
                                processor index is returned. Always 0 returned.

  @retval EFI_SUCCESS           Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  OUT UINTN                      *MaxProcessorIndex
  )
{
  *MaxProcessorIndex = 0;
  return EFI_SUCCESS;
}

/**
  Registers a function to be called back periodically in interrupt context.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor the callback function applies to.
  @param  PeriodicCallback      A pointer to a function of type PERIODIC_CALLBACK that is the main
                                periodic entry point of the debug agent.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a callback
                                function was previously registered.
  @retval EFI_OUT_OF_RESOURCES  System has insufficient memory resources to register new callback
                                function.
**/
EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK       PeriodicCallback
  )
{
  return ManageIdtEntryTable (PeriodicCallback, SYSTEM_TIMER_VECTOR);
}

/**
  Registers a function to be called when a given processor exception occurs.

  This code executes in boot services context.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor the callback function applies to.
  @param  ExceptionCallback     A pointer to a function of type EXCEPTION_CALLBACK that is called
                                when the processor exception specified by ExceptionType occurs.
  @param  ExceptionType         Specifies which processor exception to hook.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a callback
                                function was previously registered.
  @retval EFI_OUT_OF_RESOURCES  System has insufficient memory resources to register new callback
                                function.
**/
EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN EFI_EXCEPTION_TYPE          ExceptionType
  )
{
  return ManageIdtEntryTable (ExceptionCallback, ExceptionType);
}

/**
  Invalidates processor instruction cache for a memory range. Subsequent execution in this range
  causes a fresh memory fetch to retrieve code to be executed.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL instance.
  @param  ProcessorIndex        Specifies which processor's instruction cache is to be invalidated.
  @param  Start                 Specifies the physical base of the memory range to be invalidated.
  @param  Length                Specifies the minimum number of bytes in the processor's instruction
                                cache to invalidate.

  @retval EFI_SUCCESS           Always returned.

**/
EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN VOID                        *Start,
  IN UINT64                      Length
  )
{
  AsmWbinvd ();
  return EFI_SUCCESS;
}

/**
  Common piece of code that invokes the registered handlers.

  This code executes in exception context so no efi calls are allowed.
  This code is called from assembly file.

  @param  ExceptionType     Exception type
  @param  ContextRecord     System context

**/
VOID
InterruptDistrubutionHub (
  EFI_EXCEPTION_TYPE       ExceptionType,
  EFI_SYSTEM_CONTEXT_IA32  *ContextRecord
  )
{
  if (IdtEntryTable[ExceptionType].RegisteredCallback != NULL) {
    if (ExceptionType != SYSTEM_TIMER_VECTOR) {
      IdtEntryTable[ExceptionType].RegisteredCallback (ExceptionType, ContextRecord);
    } else {
      OrigVector = IdtEntryTable[ExceptionType].OrigVector;
      IdtEntryTable[ExceptionType].RegisteredCallback (ContextRecord);
    }
  }
}

/**
  This is the callback that is written to the Loaded Image protocol instance
  on the image handle. It uninstalls all registered handlers and frees all entry
  stub memory.

  @param  ImageHandle    The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    Always.

**/
EFI_STATUS
EFIAPI
PlUnloadDebugSupportDriver (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  for (ExceptionType = 0; ExceptionType < NUM_IDT_ENTRIES; ExceptionType++) {
    ManageIdtEntryTable (NULL, ExceptionType);
    //
    // Free space for each Interrupt Stub precedure.
    //
    if (IdtEntryTable[ExceptionType].StubEntry != NULL) {
      FreePool ((VOID *)(UINTN)IdtEntryTable[ExceptionType].StubEntry);
    }
  }

  FreePool (IdtEntryTable);

  return EFI_SUCCESS;
}

/**
  Initializes driver's handler registration database.

  This code executes in boot services context.
  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_UNSUPPORTED      If IA32/x64 processor does not support FXSTOR/FXRSTOR instructions,
                                the context save will fail, so these processors are not supported.
  @retval  EFI_OUT_OF_RESOURCES Fails to allocate memory.
  @retval  EFI_SUCCESS          Initializes successfully.

**/
EFI_STATUS
PlInitializeDebugSupportDriver (
  VOID
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  //
  // Check whether FxStor instructions are supported.
  //
  if (!FxStorSupport ()) {
    return EFI_UNSUPPORTED;
  }

  IdtEntryTable = AllocateZeroPool (sizeof (IDT_ENTRY) * NUM_IDT_ENTRIES);
  if (IdtEntryTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (ExceptionType = 0; ExceptionType < NUM_IDT_ENTRIES; ExceptionType++) {
    IdtEntryTable[ExceptionType].StubEntry = (DEBUG_PROC)(UINTN)AllocatePool (StubSize);
    if (IdtEntryTable[ExceptionType].StubEntry == NULL) {
      goto ErrorCleanup;
    }

    //
    // Copy Interrupt stub code.
    //
    CopyMem ((VOID *)(UINTN)IdtEntryTable[ExceptionType].StubEntry, InterruptEntryStub, StubSize);
  }

  return EFI_SUCCESS;

ErrorCleanup:

  for (ExceptionType = 0; ExceptionType < NUM_IDT_ENTRIES; ExceptionType++) {
    if (IdtEntryTable[ExceptionType].StubEntry != NULL) {
      FreePool ((VOID *)(UINTN)IdtEntryTable[ExceptionType].StubEntry);
    }
  }

  FreePool (IdtEntryTable);

  return EFI_OUT_OF_RESOURCES;
}
