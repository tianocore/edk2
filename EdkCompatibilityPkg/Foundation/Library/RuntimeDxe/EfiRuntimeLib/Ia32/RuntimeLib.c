/*++

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
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
#include "PeiHob.h"
#include EFI_PROTOCOL_DEFINITION (CpuIo)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (Hob)
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)
#include EFI_PROTOCOL_DEFINITION (SmmStatusCode)
#include EFI_PROTOCOL_DEFINITION (SmmBase)

//
// Driver Lib Module Globals
//
static EFI_RUNTIME_SERVICES *mRT;
static EFI_EVENT            mRuntimeNotifyEvent     = NULL;
static EFI_EVENT            mEfiVirtualNotifyEvent  = NULL;
static BOOLEAN              mRuntimeLibInitialized  = FALSE;
static BOOLEAN              mEfiGoneVirtual         = FALSE;
static BOOLEAN              mInSmm                  = FALSE;

//
// Runtime Global, but you should use the Lib functions
//
EFI_CPU_IO_PROTOCOL         *gCpuIo;
BOOLEAN                     mEfiAtRuntime = FALSE;
FVB_ENTRY                   *mFvbEntry;
EFI_SMM_STATUS_CODE_PROTOCOL  *gSmmStatusCodeProtocol = NULL;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

EFI_REPORT_STATUS_CODE      gReportStatusCode         = NULL;
EFI_EVENT                   gEfiStatusCodeNotifyEvent = NULL;

VOID
EFIAPI
OnStatusCodeInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                Status;
  EFI_STATUS_CODE_PROTOCOL  *StatusCode;

  Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID **) &StatusCode);
  if (!EFI_ERROR (Status)) {
    gReportStatusCode = StatusCode->ReportStatusCode;
  }
}

EFI_STATUS
GetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  )
/*++

Routine Description:

  Searches for a Protocol Interface passed from PEI through a HOB

Arguments:

  ProtocolGuid - The Protocol GUID to search for in the HOB List

  Interface    - A pointer to the interface for the Protocol GUID

Returns:

  EFI_SUCCESS   - The Protocol GUID was found and its interface is returned in Interface

  EFI_NOT_FOUND - The Protocol GUID was not found in the HOB List

--*/
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  GuidHob;

  //
  // Get Hob list
  //
  Status = EfiLibGetSystemConfigurationTable (&gEfiHobListGuid, (VOID **) &GuidHob.Raw);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status);) {
    if (END_OF_HOB_LIST (GuidHob)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    if (GET_HOB_TYPE (GuidHob) == EFI_HOB_TYPE_GUID_EXTENSION) {
      if (EfiCompareGuid (ProtocolGuid, &GuidHob.Guid->Name)) {
        Status     = EFI_SUCCESS;
        *Interface = (VOID *) *(UINTN *) (GuidHob.Guid + 1);
      }
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  return Status;
}

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

EFI_STATUS
EfiConvertInternalPointer (
  IN OUT VOID                  *Address
  )
/*++

Routine Description:

  Call EfiConvertPointer() to convert internal pointer.

Arguments:

  Address - A pointer to a pointer that is to be fixed to be the value needed
            for the new virtual address mappings being applied.

Returns:

  Status code

--*/
{
  return EfiConvertPointer (EFI_INTERNAL_POINTER, Address);
}

VOID
EFIAPI
EfiRuntimeLibFvbVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Convert all pointers in mFvbEntry after ExitBootServices.

Arguments:

  Event   - The Event that is being processed
  
  Context - Event Context

Returns:

  None

--*/
{
  UINTN Index;
  if (mFvbEntry != NULL) {
    for (Index = 0; Index < MAX_FVB_COUNT; Index++) {
      if (NULL != mFvbEntry[Index].Fvb) {
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->GetBlockSize);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->GetPhysicalAddress);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->GetVolumeAttributes);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->SetVolumeAttributes);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->Read);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->Write);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb->EraseBlocks);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].Fvb);
      }

      if (NULL != mFvbEntry[Index].FvbExtension) {
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].FvbExtension->EraseFvbCustomBlock);
        EfiConvertInternalPointer ((VOID **) &mFvbEntry[Index].FvbExtension);
      }
    }

    EfiConvertInternalPointer ((VOID **) &mFvbEntry);
  }
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

extern BOOLEAN  gEfiFvbInitialized;

VOID
EFIAPI
EfiRuntimeLibVirtualNotifyEvent (
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
  EFI_EVENT_NOTIFY  ChildNotifyEventHandler;

  if (Context != NULL) {
    ChildNotifyEventHandler = (EFI_EVENT_NOTIFY) (UINTN) Context;
    ChildNotifyEventHandler (Event, NULL);
  }

  if (gEfiFvbInitialized) {
    EfiRuntimeLibFvbVirtualNotifyEvent (Event, Context);
  }
  //
  // Update global for Runtime Services Table and IO
  //
  EfiConvertInternalPointer ((VOID **) &gCpuIo);
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  if (gReportStatusCode != NULL) {
    EfiConvertInternalPointer ((VOID **) &gReportStatusCode);
  }
#endif
  EfiConvertInternalPointer ((VOID **) &mRT);

  //
  // Clear out BootService globals
  //
  gBS             = NULL;
  gST             = NULL;
  mEfiGoneVirtual = TRUE;
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
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  VOID *Registration;
#endif

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
  //
  // Register EFI_STATUS_CODE_PROTOCOL notify function
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  OnStatusCodeInstall,
                  NULL,
                  &gEfiStatusCodeNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->RegisterProtocolNotify (
                  &gEfiStatusCodeRuntimeProtocolGuid,
                  gEfiStatusCodeNotifyEvent,
                  &Registration
                  );
  ASSERT_EFI_ERROR (Status);

  gBS->SignalEvent (gEfiStatusCodeNotifyEvent);
#endif

  Status = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, (VOID **) &gCpuIo);
  if (EFI_ERROR (Status)) {
    gCpuIo = NULL;
  }

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
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  EFI_TPL_NOTIFY,
                  EfiRuntimeLibVirtualNotifyEvent,
                  (VOID *) (UINTN) GoVirtualChildEvent,
                  &mEfiVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

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

  //
  // Close SetVirtualAddressMap () notify function
  //
  if (mEfiVirtualNotifyEvent != NULL) {
    Status = gBS->CloseEvent (mEfiVirtualNotifyEvent);
    ASSERT_EFI_ERROR (Status);
  }

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  //
  // Close EfiStatusCodeRuntimeProtocol notify function
  //
  if (gEfiStatusCodeNotifyEvent != NULL) {
    Status = gBS->CloseEvent (gEfiStatusCodeNotifyEvent);
    ASSERT_EFI_ERROR (Status);
  }
#endif

  return EFI_SUCCESS;
}

EFI_STATUS
EfiInitializeSmmDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize runtime Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS except EFI_ALREADY_STARTED if already started.

--*/
{
  EFI_STATUS               Status;
  EFI_SMM_BASE_PROTOCOL    *SmmBase;

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

  //
  // Check whether it is in SMM mode.
  //
  Status  = gBS->LocateProtocol (&gEfiSmmBaseProtocolGuid, NULL, (VOID**) &SmmBase);
  if (!EFI_ERROR (Status)) {
    SmmBase->InSmm (SmmBase, &mInSmm);
  }

  //
  // Directly locate SmmStatusCode protocol
  //
  Status = gBS->LocateProtocol (&gEfiSmmStatusCodeProtocolGuid, NULL, (VOID**) &gSmmStatusCodeProtocol);
  if (EFI_ERROR (Status)) {
    gSmmStatusCodeProtocol = NULL;
  }

  Status  = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, (VOID **) &gCpuIo);
  if (EFI_ERROR (Status)) {
    gCpuIo = NULL;
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
//
// The following functions hide the mRT local global from the call to
// runtime service in the EFI system table.
//
EFI_STATUS
EfiGetTime (
  OUT EFI_TIME                    *Time,
  OUT EFI_TIME_CAPABILITIES       *Capabilities
  )
/*++

Routine Description:

  Returns the current time and date information, and the time-keeping 
  capabilities of the hardware platform.

Arguments:

  Time          - A pointer to storage to receive a snapshot of the current time.
  Capabilities  - An optional pointer to a buffer to receive the real time clock device's
                  capabilities.

Returns:

  Status code

--*/
{
  return mRT->GetTime (Time, Capabilities);
}

EFI_STATUS
EfiSetTime (
  IN EFI_TIME                   *Time
  )
/*++

Routine Description:

  Sets the current local time and date information.

Arguments:

  Time  - A pointer to the current time.

Returns:

  Status code

--*/
{
  return mRT->SetTime (Time);
}

EFI_STATUS
EfiGetWakeupTime (
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    *Time
  )
/*++

Routine Description:

  Returns the current wakeup alarm clock setting.

Arguments:

  Enabled - Indicates if the alarm is currently enabled or disabled.
  Pending - Indicates if the alarm signal is pending and requires acknowledgement.
  Time    - The current alarm setting.

Returns:

  Status code

--*/
{
  return mRT->GetWakeupTime (Enabled, Pending, Time);
}

EFI_STATUS
EfiSetWakeupTime (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     *Time
  )
/*++

Routine Description:

  Sets the system wakeup alarm clock time.

Arguments:

  Enable  - Enable or disable the wakeup alarm.
  Time    - If Enable is TRUE, the time to set the wakeup alarm for.
            If Enable is FALSE, then this parameter is optional, and may be NULL.

Returns:

  Status code

--*/
{
  return mRT->SetWakeupTime (Enable, Time);
}

EFI_STATUS
EfiGetVariable (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  )
/*++

Routine Description:

  Returns the value of a variable.

Arguments:

  VariableName  - A Null-terminated Unicode string that is the name of the
                  vendor's variable.
  VendorGuid    - A unique identifier for the vendor.
  Attributes    - If not NULL, a pointer to the memory location to return the
                  attributes bitmask for the variable.
  DataSize      - On input, the size in bytes of the return Data buffer.
                  On output the size of data returned in Data.
  Data          - The buffer to return the contents of the variable.

Returns:

  Status code

--*/
{
  return mRT->GetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
}

EFI_STATUS
EfiGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
/*++

Routine Description:

  Enumerates the current variable names.

Arguments:

  VariableNameSize  - The size of the VariableName buffer.
  VariableName      - On input, supplies the last VariableName that was returned
                      by GetNextVariableName(). 
                      On output, returns the Nullterminated Unicode string of the
                      current variable.
  VendorGuid        - On input, supplies the last VendorGuid that was returned by
                      GetNextVariableName(). 
                      On output, returns the VendorGuid of the current variable.

Returns:

  Status code

--*/
{
  return mRT->GetNextVariableName (VariableNameSize, VariableName, VendorGuid);
}

EFI_STATUS
EfiSetVariable (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     *VendorGuid,
  IN UINT32                       Attributes,
  IN UINTN                        DataSize,
  IN VOID                         *Data
  )
/*++

Routine Description:

  Sets the value of a variable.

Arguments:

  VariableName  - A Null-terminated Unicode string that is the name of the
                  vendor's variable.
  VendorGuid    - A unique identifier for the vendor.
  Attributes    - Attributes bitmask to set for the variable.
  DataSize      - The size in bytes of the Data buffer.
  Data          - The contents for the variable.

Returns:

  Status code

--*/
{
  return mRT->SetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
}


#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

EFI_STATUS
EfiQueryVariableInfo (
  IN UINT32           Attributes,
  OUT UINT64          *MaximumVariableStorageSize,
  OUT UINT64          *RemainingVariableStorageSize,
  OUT UINT64          *MaximumVariableSize
  )

/*++

Routine Description:

  This code returns information about the EFI variables.

Arguments:

  Attributes                      Attributes bitmask to specify the type of variables 
                                  on which to return information.
  MaximumVariableStorageSize      Pointer to the maximum size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  RemainingVariableStorageSize    Pointer to the remaining size of the storage space available 
                                  for the EFI variables associated with the attributes specified.
  MaximumVariableSize             Pointer to the maximum size of the individual EFI variables
                                  associated with the attributes specified.

Returns:

  Status code

--*/
{
  return mRT->QueryVariableInfo (Attributes, MaximumVariableStorageSize, RemainingVariableStorageSize, MaximumVariableSize);
}

#endif

EFI_STATUS
EfiGetNextHighMonotonicCount (
  OUT UINT32                      *HighCount
  )
/*++

Routine Description:

  Returns the next high 32 bits of the platform's monotonic counter.

Arguments:

  HighCount - Pointer to returned value.

Returns:

  Status code

--*/
{
  return mRT->GetNextHighMonotonicCount (HighCount);
}

VOID
EfiResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN CHAR16                       *ResetData
  )
/*++

Routine Description:

  Resets the entire platform.

Arguments:

  ResetType   - The type of reset to perform.
  ResetStatus - The status code for the reset.
  DataSize    - The size, in bytes, of ResetData.
  ResetData   - A data buffer that includes a Null-terminated Unicode string, optionally
                followed by additional binary data.

Returns:

  None

--*/
{
  mRT->ResetSystem (ResetType, ResetStatus, DataSize, ResetData);
}

EFI_STATUS
EfiReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL  
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
  EFI_STATUS               Status;
  
  if (mInSmm) {
    if (gSmmStatusCodeProtocol == NULL) {
      return EFI_UNSUPPORTED;
    }
    Status = gSmmStatusCodeProtocol->ReportStatusCode (gSmmStatusCodeProtocol, CodeType, Value, Instance, CallerId, Data);
    return Status;
  }

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  if (gReportStatusCode == NULL) {
    //
    // Because we've installed the protocol notification on EfiStatusCodeRuntimeProtocol,
    //   running here indicates that the StatusCode driver has not started yet.
    //
    if (EfiAtRuntime ()) {
      //
      // Running here only when StatusCode driver never starts.
      //
      return EFI_UNSUPPORTED;
    }

    //
    // Try to get the PEI version of ReportStatusCode.
    //
    Status = GetPeiProtocol (&gEfiStatusCodeRuntimeProtocolGuid, (VOID **) &gReportStatusCode);
    if (EFI_ERROR (Status) || (gReportStatusCode == NULL)) {
      return EFI_UNSUPPORTED;
    }
  }
  Status = gReportStatusCode (CodeType, Value, Instance, CallerId, Data);
#else
  if (mRT == NULL) {
    return EFI_UNSUPPORTED;
  }
  //
  // Check whether EFI_RUNTIME_SERVICES has Tiano Extension
  //
  Status = EFI_UNSUPPORTED;
  if (mRT->Hdr.Revision     == EFI_SPECIFICATION_VERSION     &&
      mRT->Hdr.HeaderSize   == sizeof (EFI_RUNTIME_SERVICES) &&
      mRT->ReportStatusCode != NULL) {
    Status = mRT->ReportStatusCode (CodeType, Value, Instance, CallerId, Data);
  }
#endif
  return Status;
}
