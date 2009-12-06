/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
/** @file
  DXE Cpu Driver.
  
  May need some porting work for platform specifics.

  Copyright (c) 2008, Apple Inc                                                        
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "CpuDxe.h"

EFI_PERIODIC_CALLBACK   gPeriodicCallBack = (EFI_PERIODIC_CALLBACK)NULL;

EFI_DEBUG_SUPPORT_PERIODIC_CALLBACK_PROTOCOL *gDebugSupportCallback = NULL;


EFI_STATUS
EFIAPI
DebugSupportGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  OUT UINTN                           *MaxProcessorIndex
  )
/*++

Routine Description: This is a DebugSupport protocol member function.

Arguments:
  This              - The DebugSupport instance
  MaxProcessorIndex - The maximuim supported processor index

Returns:
  Always returns EFI_SUCCESS with *MaxProcessorIndex set to 0

--*/
{
  *MaxProcessorIndex = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DebugSupportRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL *This,
  IN UINTN                      ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK      PeriodicCallback
  )
/*++

Routine Description: This is a DebugSupport protocol member function.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  PeriodicCallback - Callback function

Returns:

  EFI_SUCCESS
  EFI_INVALID_PARAMETER - requested uninstalling a handler from a vector that has
                          no handler registered for it
  EFI_ALREADY_STARTED   - requested install to a vector that already has a handler registered.

  Other possible return values are passed through from UnHookEntry and HookEntry.

--*/
{
  if (ProcessorIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((gPeriodicCallBack != (EFI_PERIODIC_CALLBACK)NULL) && (PeriodicCallback != (EFI_PERIODIC_CALLBACK)NULL)) {
    return EFI_ALREADY_STARTED;
  }
  
  gPeriodicCallBack = PeriodicCallback;
  
  if (gDebugSupportCallback != NULL) {
    //
    // We can only update this protocol if the Register Protocol Notify has fired. If it fires 
    // after this call it will update with gPeriodicCallBack value.
    //
    gDebugSupportCallback->PeriodicCallback = gPeriodicCallBack;
  }
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
DebugSupportRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL *This,
  IN UINTN                      ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK     NewCallback,
  IN EFI_EXCEPTION_TYPE         ExceptionType
  )
/*++

Routine Description:
  This is a DebugSupport protocol member function.

  This code executes in boot services context.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  NewCallback      - Callback function
  ExceptionType    - Which exception to hook

Returns:

  EFI_SUCCESS
  EFI_INVALID_PARAMETER - requested uninstalling a handler from a vector that has
                          no handler registered for it
  EFI_ALREADY_STARTED   - requested install to a vector that already has a handler registered.

  Other possible return values are passed through from UnHookEntry and HookEntry.

--*/
{
  if (ProcessorIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return RegisterDebuggerInterruptHandler (ExceptionType, NewCallback);
}


EFI_STATUS
EFIAPI
DebugSupportInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL       *This,
  IN UINTN                            ProcessorIndex,
  IN VOID                             *Start,
  IN UINT64                           Length
  )
/*++

Routine Description:
  This is a DebugSupport protocol member function.
  Calls assembly routine to flush cache.

Arguments:
  This             - The DebugSupport instance
  ProcessorIndex   - Which processor the callback applies to.
  Start            - Physical base of the memory range to be invalidated
  Length           - mininum number of bytes in instruction cache to invalidate

Returns:

  EFI_SUCCESS - always return success

--*/
{
  if (ProcessorIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  InvalidateInstructionCache();

  return EFI_SUCCESS;
}

//
// This is a global that is the actual interface
//
EFI_DEBUG_SUPPORT_PROTOCOL  gDebugSupportProtocolInterface = {
  IsaArm, // Fixme to be more generic
  DebugSupportGetMaximumProcessorIndex,
  DebugSupportRegisterPeriodicCallback,
  DebugSupportRegisterExceptionCallback,
  DebugSupportInvalidateInstructionCache
};


VOID
EFIAPI
DebugSupportPeriodicCallbackEventProtocolNotify (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS    Status;
  
  Status = gBS->LocateProtocol (&gEfiDebugSupportPeriodicCallbackProtocolGuid, NULL, (VOID **)&gDebugSupportCallback);
  if (!EFI_ERROR (Status)) {
    gDebugSupportCallback->PeriodicCallback = gPeriodicCallBack;
  }
}

VOID *gRegistration = NULL;


EFI_DEBUG_SUPPORT_PROTOCOL *
InitilaizeDebugSupport (
  VOID
  )
{
  // RPN gEfiDebugSupportPeriodicCallbackProtocolGuid
  EFI_STATUS    Status;
  EFI_EVENT     Event;

  if (!FeaturePcdGet (PcdCpuDxeProduceDebugSupport)) {
    // Don't include this code unless Feature Flag is set
    return NULL;
  }
  

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL, 
                  TPL_CALLBACK, 
                  DebugSupportPeriodicCallbackEventProtocolNotify, 
                  NULL, 
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (&gEfiDebugSupportPeriodicCallbackProtocolGuid, Event, &gRegistration);
  ASSERT_EFI_ERROR (Status);

  //
  // We assume the Timer must depend on our driver to register interrupts so we don't need to do
  // a gBS->SignalEvent (Event) here to check to see if the protocol allready exists
  //

  return &gDebugSupportProtocolInterface;
}
