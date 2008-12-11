/** @file
  IA32 specific debug support functions

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// private header files
//
#include "DebugSupport.h"

//
// This the global main table to keep track of the interrupts
//
IDT_ENTRY   *IdtEntryTable  = NULL;
IA32_IDT_GATE_DESCRIPTOR  NullDesc        = {0};

/**
  Read IDT Gate Descriptor from IDT Table.

  @param  Vector            Specifies vector number.
  @param  IdtGateDecriptor  Pointer to IDT Gate Descriptor read from IDT Table.

**/
VOID ReadIdtGateDecriptor (
  IN  EFI_EXCEPTION_TYPE        Vector,
  OUT IA32_IDT_GATE_DESCRIPTOR  *IdtGateDecriptor
  )
{
 IA32_DESCRIPTOR            IdtrValue;
 IA32_IDT_GATE_DESCRIPTOR   *IdtTable;

 AsmReadIdtr (&IdtrValue);
 IdtTable = (IA32_IDT_GATE_DESCRIPTOR *) IdtrValue.Base;

 CopyMem ((VOID *) IdtGateDecriptor, (VOID *) &(IdtTable)[Vector], sizeof (IA32_IDT_GATE_DESCRIPTOR));
}
/**
  Write IDT Gate Descriptor into IDT Table.

  @param  Vector            Specifies vector number.
  @param  IdtGateDecriptor  Pointer to IDT Gate Descriptor written into IDT Table.

**/
VOID WriteIdtGateDecriptor (
  EFI_EXCEPTION_TYPE        Vector,
  IA32_IDT_GATE_DESCRIPTOR  *IdtGateDecriptor
  )
{
 IA32_DESCRIPTOR            IdtrValue;
 IA32_IDT_GATE_DESCRIPTOR   *IdtTable;

 AsmReadIdtr (&IdtrValue);
 IdtTable = (IA32_IDT_GATE_DESCRIPTOR *) IdtrValue.Base;

 CopyMem ((VOID *) &(IdtTable)[Vector], (VOID *) IdtGateDecriptor, sizeof (IA32_IDT_GATE_DESCRIPTOR));
}


/**
  Creates a nes entry stub.  Then saves the current IDT entry and replaces it
  with an interrupt gate for the new entry point.  The IdtEntryTable is updated
  with the new registered function.

  This code executes in boot services context.  The stub entry executes in interrupt
  context.

  @param  ExceptionType      Specifies which vector to hook.
  @param  NewCallback        A pointer to the new function to be registered.

  @retval EFI_SUCCESS        Always.

**/
EFI_STATUS
HookEntry (
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN VOID                         (*NewCallback) ()
  )
{
  BOOLEAN     OldIntFlagState;
  EFI_STATUS  Status;

  Status = CreateEntryStub (ExceptionType, (VOID **) &IdtEntryTable[ExceptionType].StubEntry);
  if (Status == EFI_SUCCESS) {
    OldIntFlagState = WriteInterruptFlag (0);
    ReadIdtGateDecriptor (ExceptionType, &(IdtEntryTable[ExceptionType].OrigDesc));

    IdtEntryTable[ExceptionType].OrigVector = (DEBUG_PROC) GetProcedureEntryPoint (&(IdtEntryTable[ExceptionType].OrigDesc));

    Vect2Desc (&IdtEntryTable[ExceptionType].NewDesc, IdtEntryTable[ExceptionType].StubEntry);
    IdtEntryTable[ExceptionType].RegisteredCallback = NewCallback;
    WriteIdtGateDecriptor (ExceptionType, &(IdtEntryTable[ExceptionType].NewDesc));
    WriteInterruptFlag (OldIntFlagState);
  }

  return Status;
}

/**
  Undoes HookEntry. This code executes in boot services context.

  @param  ExceptionType   Specifies which entry to unhook

  @retval EFI_SUCCESS     Always.

**/
EFI_STATUS
UnhookEntry (
  IN EFI_EXCEPTION_TYPE           ExceptionType
  )
{
  BOOLEAN     OldIntFlagState;

  OldIntFlagState = WriteInterruptFlag (0);
  WriteIdtGateDecriptor (ExceptionType, &(IdtEntryTable[ExceptionType].OrigDesc));
  WriteInterruptFlag (OldIntFlagState);

  return EFI_SUCCESS;
}

/**
  This is a DebugSupport protocol member function, hard
  coded to support only 1 processor for now.

  @param  This                The DebugSupport instance
  @param  MaxProcessorIndex   The maximuim supported processor index

  @retval EFI_SUCCESS         Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  OUT UINTN                           *MaxProcessorIndex
  )
{
  *MaxProcessorIndex = 0;
  return (EFI_SUCCESS);
}

/**
  DebugSupport protocol member function.

  @param  This               The DebugSupport instance
  @param  ProcessorIndex     Which processor the callback applies to.
  @param  PeriodicCallback   Callback function

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL *This,
  IN UINTN                      ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK      PeriodicCallback
  )
{
  return ManageIdtEntryTable (PeriodicCallback, SYSTEM_TIMER_VECTOR);
}

/**
  DebugSupport protocol member function.

  This code executes in boot services context.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  NewCallback       Callback function
  @param  ExceptionType     Which exception to hook

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL *This,
  IN UINTN                      ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK     NewCallback,
  IN EFI_EXCEPTION_TYPE         ExceptionType
  )
{
  return ManageIdtEntryTable (NewCallback, ExceptionType);
}

/**
  DebugSupport protocol member function.  Calls assembly routine to flush cache.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  Start             Physical base of the memory range to be invalidated
  @param  Length            mininum number of bytes in instruction cache to invalidate

  @retval EFI_SUCCESS       Always returned.

**/
EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN VOID                             *Start,
  IN UINT64                           Length
  )
{
  AsmWbinvd ();
  return EFI_SUCCESS;
}

/**
  Initializes driver's handler registration databas. 
  
  This code executes in boot services context
  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_UNSUPPORTED      If IA32 processor does not support FXSTOR/FXRSTOR instructions,
                                the context save will fail, so these processor's are not supported.
  @retval  EFI_OUT_OF_RESOURCES Fails to allocate memory.
  @retval  EFI_SUCCESS          Initializes successfully.

**/
EFI_STATUS
PlInitializeDebugSupportDriver (
  VOID
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  if (!FxStorSupport ()) {
    return EFI_UNSUPPORTED;
  }

  IdtEntryTable = AllocateZeroPool (sizeof (IDT_ENTRY) * NUM_IDT_ENTRIES);
  if (IdtEntryTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (ExceptionType = 0; ExceptionType < NUM_IDT_ENTRIES; ExceptionType++) {
    IdtEntryTable[ExceptionType].StubEntry = (DEBUG_PROC) (UINTN) AllocatePool (StubSize);
    if (IdtEntryTable[ExceptionType].StubEntry == NULL) {
      goto ErrorCleanup;
    }

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

/**
  This is the callback that is written to the LoadedImage protocol instance
  on the image handle. It uninstalls all registered handlers and frees all entry
  stub memory.

  @param  ImageHandle    The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    Always.

**/
EFI_STATUS
EFIAPI
PlUnloadDebugSupportDriver (
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  for (ExceptionType = 0; ExceptionType < NUM_IDT_ENTRIES; ExceptionType++) {
    ManageIdtEntryTable (NULL, ExceptionType);
  }

  FreePool (IdtEntryTable);
  return EFI_SUCCESS;
}

/**
  Common piece of code that invokes the registered handlers.

  This code executes in exception context so no efi calls are allowed.

  @param  ExceptionType     Exception type
  @param  ContextRecord     System context

**/
VOID
InterruptDistrubutionHub (
  EFI_EXCEPTION_TYPE      ExceptionType,
  EFI_SYSTEM_CONTEXT_IA32 *ContextRecord
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
