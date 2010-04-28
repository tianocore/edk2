/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    RuntimeLib.c

Abstract:

  Light weight lib to support Tiano drivers.

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)

//
// Driver Lib Module Globals
//
static EFI_RUNTIME_SERVICES *mRT;
static EFI_EVENT            mRuntimeNotifyEvent     = NULL;
static BOOLEAN              mRuntimeLibInitialized  = FALSE;
static BOOLEAN              mEfiGoneVirtual         = FALSE;

//
// Runtime Global, but you should use the Lib functions
//
BOOLEAN                     mEfiAtRuntime = FALSE;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
static EFI_STATUS_CODE_PROTOCOL  *gStatusCode = NULL;
#endif

EFI_STATUS
EfiConvertPointer (
  IN UINTN                     DebugDisposition,
  IN OUT VOID                  *Address
  )
/*++

Routine Description:

  Determines the new virtual address that is to be used on subsequent memory accesses.

Arguments:

  DebugDisposition  - Supplies type information for the pointer being converted.
  Address           - A pointer to a pointer that is to be fixed to be the value needed
                      for the new virtual address mappings being applied.

Returns:

  Status code

--*/
{
  return mRT->ConvertPointer (DebugDisposition, Address);
}

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
  mEfiAtRuntime = TRUE;
}

EFI_STATUS
EfiInitializeRuntimeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN EFI_EVENT_NOTIFY     GoVirtualChildEvent
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

  if (mRuntimeLibInitialized) {
    return EFI_ALREADY_STARTED;
  }

  mRuntimeLibInitialized  = TRUE;

  gST = SystemTable;
  ASSERT (gST != NULL);

  gBS = SystemTable->BootServices;
  ASSERT (gBS != NULL);
  mRT = SystemTable->RuntimeServices;
  ASSERT (mRT != NULL);

  Status  = EfiLibGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);
  ASSERT_EFI_ERROR (Status);

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID **)&gStatusCode);
  if (EFI_ERROR (Status)) {
    gStatusCode = NULL;
  }
#endif

  //
  // Register our ExitBootServices () notify function
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
                  EFI_TPL_NOTIFY,
                  RuntimeDriverExitBootServices,
                  NULL,
                  &mRuntimeNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // To NOT register SetVirtualAddressMap () notify function,
  // because we do not know how to trigger it without our EBC driver.
  //

  return EFI_SUCCESS;
}

EFI_STATUS
EfiShutdownRuntimeDriverLib (
  VOID
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

  if (!mRuntimeLibInitialized) {
    //
    // You must call EfiInitializeRuntimeDriverLib() first
    //
    return EFI_UNSUPPORTED;
  }

  mRuntimeLibInitialized = FALSE;

  //
  // Close our ExitBootServices () notify function
  //
  if (mRuntimeNotifyEvent != NULL) {
    Status = gBS->CloseEvent (mRuntimeNotifyEvent);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

BOOLEAN
EfiAtRuntime (
  VOID
  )
/*++

Routine Description:
  Return TRUE if ExitBootServices () has been called

Arguments:
  NONE

Returns: 
  TRUE - If ExitBootServices () has been called

--*/
{
  return mEfiAtRuntime;
}

BOOLEAN
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
  return mEfiGoneVirtual;
}

EFI_STATUS
EfiReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Status Code reporter

Arguments:

  CodeType    - Type of Status Code.
  
  Value       - Value to output for Status Code.
  
  Instance    - Instance Number of this status code.
  
  CallerId    - ID of the caller of this status code.
  
  Data        - Optional data associated with this status code.

Returns:

  Status code

--*/
{
  return EFI_UNSUPPORTED;
}
//
// Cache Flush Routine.
//
EFI_STATUS
EfiCpuFlushCache (
  IN EFI_PHYSICAL_ADDRESS          Start,
  IN UINT64                        Length
  )
/*++

Routine Description:

  Flush cache with specified range.

Arguments:

  Start   - Start address
  Length  - Length in bytes

Returns:

  Status code
  
  EFI_SUCCESS - success

--*/
{
  return EFI_SUCCESS;
}
