/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.           


Module Name:

  RuntimeLib.c

Abstract:

  Runtime lib

--*/

#include "../RuntimeLibInternal.h"

//
// Driver Lib Module Globals
//
static EFI_EVENT      mEfiVirtualNotifyEvent;
EFI_RUNTIME_SERVICES  *mRTEdkDxeRuntimeDriverLib;

VOID
EFIAPI
RuntimeDriverExitBootServices (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Set AtRuntime flag as TRUE after ExitBootServices

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns: 

  None

--*/
{
  if (EfiAtRuntime()) {
    return;
  }
}

STATIC
VOID
EFIAPI
RuntimeLibVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in 
  lib to virtual mode.

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns: 

  None

--*/
{
  UINTN             Index;
  EFI_EVENT_NOTIFY  ChildNotifyEventHandler;

  for (Index = 0; _gDriverSetVirtualAddressMapEvent[Index] != NULL; Index++) {
    ChildNotifyEventHandler = _gDriverSetVirtualAddressMapEvent[Index];
    ChildNotifyEventHandler (Event, NULL);
  }

  //
  // Update global for Runtime Services Table
  //
  EfiConvertPointer (0, (VOID **) &mRTEdkDxeRuntimeDriverLib);
}

EFI_STATUS
EFIAPI
RuntimeDriverLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize runtime Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

  GoVirtualChildEvent - Caller can register a virtual notification event.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS except EFI_ALREADY_STARTED if already started.

--*/
{
  EFI_STATUS  Status;

  mRTEdkDxeRuntimeDriverLib = SystemTable->RuntimeServices;

  //
  // Register SetVirtualAddressMap () notify function
  //
  if (_gDriverSetVirtualAddressMapEvent[0] != NULL) {
    Status = gBS->CreateEvent (
                    EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                    EFI_TPL_NOTIFY,
                    RuntimeLibVirtualNotifyEvent,
                    NULL,
                    &mEfiVirtualNotifyEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RuntimeDriverLibDeconstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  This routine will free some resources which have been allocated in
  EfiInitializeRuntimeDriverLib(). If a runtime driver exits with an error, 
  it must call this routine to free the allocated resource before the exiting.

Arguments:

  None

Returns: 

  EFI_SUCCESS     - Shotdown the Runtime Driver Lib successfully
  EFI_UNSUPPORTED - Runtime Driver lib was not initialized at all

--*/
{
  EFI_STATUS  Status;

  //
  // Close SetVirtualAddressMap () notify function
  //
  if (_gDriverSetVirtualAddressMapEvent[0] != NULL) {
    Status = gBS->CloseEvent (mEfiVirtualNotifyEvent);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
EfiAtRuntime (
  VOID
  )
/*++

Routine Description:
  Return TRUE if ExitBootService () has been called

Arguments:
  NONE

Returns: 
  TRUE - If ExitBootService () has been called

--*/
{
  EFI_GUID Guid;
  SAL_RETURN_REGS ReturnReg;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, IsEfiRuntime, 0, 0, 0, 0, 0, 0, 0);

  return (BOOLEAN) (ReturnReg.r9 == 1);
}

BOOLEAN
EFIAPI
EfiGoneVirtual (
  VOID
  )
/*++

Routine Description:
  Return TRUE if SetVirtualAddressMap () has been called

Arguments:
  NONE

Returns: 
  TRUE - If SetVirtualAddressMap () has been called

--*/
{
  EFI_GUID Guid;
  SAL_RETURN_REGS ReturnReg;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, IsVirtual, 0, 0, 0, 0, 0, 0, 0);

  return (BOOLEAN) (ReturnReg.r9 == 1);
}

