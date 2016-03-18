/** @file
  X64 specific functions to support Debug Support protocol.

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PlDebugSupport.h"

IA32_IDT_GATE_DESCRIPTOR  NullDesc = {{0,0}};

/**
  Get Interrupt Handle from IDT Gate Descriptor.

  @param  IdtGateDecriptor  IDT Gate Descriptor.

  @return Interrupt Handle stored in IDT Gate Descriptor.

**/
UINTN
GetInterruptHandleFromIdt (
  IN IA32_IDT_GATE_DESCRIPTOR  *IdtGateDecriptor
  )
{
  UINTN      InterruptHandle;

  //
  // InterruptHandle  0-15 : OffsetLow
  // InterruptHandle 16-31 : OffsetHigh
  // InterruptHandle 32-63 : OffsetUpper
  //
  InterruptHandle = ((UINTN) IdtGateDecriptor->Bits.OffsetLow) |
                    (((UINTN) IdtGateDecriptor->Bits.OffsetHigh)  << 16) |
                    (((UINTN) IdtGateDecriptor->Bits.OffsetUpper) << 32) ;

  return InterruptHandle;
}

/**
  Allocate pool for a new IDT entry stub.

  Copy the generic stub into the new buffer and fixup the vector number
  and jump target address.

  @param  ExceptionType   This is the exception type that the new stub will be created
                          for.
  @param  Stub            On successful exit, *Stub contains the newly allocated entry stub.

**/
VOID
CreateEntryStub (
  IN EFI_EXCEPTION_TYPE     ExceptionType,
  OUT VOID                  **Stub
  )
{
  UINT8       *StubCopy;

  StubCopy = *Stub;

  //
  // Fixup the stub code for this vector
  //

  // The stub code looks like this:
  //
  //    00000000  6A 00               push    0                       ; push vector number - will be modified before installed
  //    00000002  E9                  db      0e9h                    ; jump rel32
  //    00000003  00000000            dd      0                       ; fixed up to relative address of CommonIdtEntry
  //

  //
  // poke in the exception type so the second push pushes the exception type
  //
  StubCopy[0x1] = (UINT8) ExceptionType;

  //
  // fixup the jump target to point to the common entry
  //
  *(UINT32 *) &StubCopy[0x3] = (UINT32)((UINTN) CommonIdtEntry - (UINTN) &StubCopy[StubSize]);

  return;
}

/**
  This is the main worker function that manages the state of the interrupt
  handlers.  It both installs and uninstalls interrupt handlers based on the
  value of NewCallback.  If NewCallback is NULL, then uninstall is indicated.
  If NewCallback is non-NULL, then install is indicated.

  @param  NewCallback   If non-NULL, NewCallback specifies the new handler to register.
                        If NULL, specifies that the previously registered handler should
                        be uninstalled.
  @param  ExceptionType Indicates which entry to manage.

  @retval EFI_SUCCESS            Process is ok.
  @retval EFI_INVALID_PARAMETER  Requested uninstalling a handler from a vector that has
                                 no handler registered for it
  @retval EFI_ALREADY_STARTED    Requested install to a vector that already has a handler registered.
  @retval others                 Possible return values are passed through from UnHookEntry and HookEntry.

**/
EFI_STATUS
ManageIdtEntryTable (
  CALLBACK_FUNC      NewCallback,
  EFI_EXCEPTION_TYPE ExceptionType
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (CompareMem (&IdtEntryTable[ExceptionType].NewDesc, &NullDesc, sizeof (IA32_IDT_GATE_DESCRIPTOR)) != 0) {
    //
    // we've already installed to this vector
    //
    if (NewCallback != NULL) {
      //
      // if the input handler is non-null, error
      //
      Status = EFI_ALREADY_STARTED;
    } else {
      UnhookEntry (ExceptionType);
    }
  } else {
    //
    // no user handler installed on this vector
    //
    if (NewCallback == NULL) {
      //
      // if the input handler is null, error
      //
      Status = EFI_INVALID_PARAMETER;
    } else {
      HookEntry (ExceptionType, NewCallback);
    }
  }

  return Status;
}
