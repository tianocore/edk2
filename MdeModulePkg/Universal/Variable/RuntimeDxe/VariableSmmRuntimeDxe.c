/** @file

  Implement all four UEFI Runtime Variable services for the nonvolatile
  and volatile storage space and install variable architecture protocol
  based on SMM variable module.

Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/
#include <PiDxe.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Variable.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmVariable.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include <Guid/EventGroup.h>
#include <Guid/VariableFormat.h>
#include <Guid/SmmVariableCommon.h>

EFI_HANDLE                       mHandle                    = NULL; 
EFI_SMM_VARIABLE_PROTOCOL       *mSmmVariable               = NULL;
EFI_EVENT                        mVirtualAddressChangeEvent = NULL;
EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication          = NULL;
UINT8                           *mVariableBuffer            = NULL;
UINT8                           *mVariableBufferPhysical    = NULL;
UINTN                            mVariableBufferSize;
UINTN                            mVariableBufferPayloadSize;
EFI_LOCK                         mVariableServicesLock;

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK                             *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK                             *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Initialize the communicate buffer using DataSize and Function.

  The communicate size is: SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE +
  DataSize.

  @param[out]      DataPtr          Points to the data in the communicate buffer.
  @param[in]       DataSize         The data size to send to SMM.
  @param[in]       Function         The function number to initialize the communicate header.
                      
  @retval EFI_INVALID_PARAMETER     The data size is too big.
  @retval EFI_SUCCESS               Find the specified variable.

**/
EFI_STATUS
InitCommunicateBuffer (
  OUT     VOID                              **DataPtr OPTIONAL,
  IN      UINTN                             DataSize,
  IN      UINTN                             Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER                *SmmCommunicateHeader;  
  SMM_VARIABLE_COMMUNICATE_HEADER           *SmmVariableFunctionHeader; 

 
  if (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE > mVariableBufferSize) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *) mVariableBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
   
  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *) SmmCommunicateHeader->Data;
  SmmVariableFunctionHeader->Function = Function;
  if (DataPtr != NULL) {
    *DataPtr = SmmVariableFunctionHeader->Data;
  }

  return EFI_SUCCESS;
}


/**
  Send the data in communicate buffer to SMM.

  @param[in]   DataSize               This size of the function header and the data.

  @retval      EFI_SUCCESS            Success is returned from the functin in SMM.
  @retval      Others                 Failure is returned from the function in SMM. 
  
**/
EFI_STATUS
SendCommunicateBuffer (
  IN      UINTN                             DataSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     CommSize;
  EFI_SMM_COMMUNICATE_HEADER                *SmmCommunicateHeader;  
  SMM_VARIABLE_COMMUNICATE_HEADER           *SmmVariableFunctionHeader;
  
  CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  Status = mSmmCommunication->Communicate (mSmmCommunication, mVariableBufferPhysical, &CommSize);
  ASSERT_EFI_ERROR (Status);

  SmmCommunicateHeader      = (EFI_SMM_COMMUNICATE_HEADER *) mVariableBuffer;
  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
  return  SmmVariableFunctionHeader->ReturnStatus;
}


/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param[in]      VariableName       Name of Variable to be found.
  @param[in]      VendorGuid         Variable vendor GUID.
  @param[out]     Attributes         Attribute value of the variable found.
  @param[in, out] DataSize           Size of Data found. If size is less than the
                                     data, this value contains the required size.
  @param[out]     Data               Data pointer.
                      
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_SUCCESS                Find the specified variable.
  @retval EFI_NOT_FOUND              Not found.
  @retval EFI_BUFFER_TO_SMALL        DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetVariable (
  IN      CHAR16                            *VariableName,
  IN      EFI_GUID                          *VendorGuid,
  OUT     UINT32                            *Attributes OPTIONAL,
  IN OUT  UINTN                             *DataSize,
  OUT     VOID                              *Data
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *SmmVariableHeader;
  UINTN                                     TempDataSize;
  UINTN                                     VariableNameSize;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TempDataSize          = *DataSize;
  VariableNameSize      = StrSize (VariableName);

  //
  // If VariableName exceeds SMM payload limit. Return failure
  //
  if (VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  if (TempDataSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) - VariableNameSize) {
    //
    // If output data buffer exceed SMM payload limit. Trim output buffer to SMM payload size
    //
    TempDataSize = mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) - VariableNameSize;
  }
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + VariableNameSize + TempDataSize;

  Status = InitCommunicateBuffer ((VOID **)&SmmVariableHeader, PayloadSize, SMM_VARIABLE_FUNCTION_GET_VARIABLE);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  ASSERT (SmmVariableHeader != NULL);

  CopyGuid (&SmmVariableHeader->Guid, VendorGuid);
  SmmVariableHeader->DataSize   = TempDataSize;
  SmmVariableHeader->NameSize   = VariableNameSize;
  if (Attributes == NULL) {
    SmmVariableHeader->Attributes = 0;
  } else {
    SmmVariableHeader->Attributes = *Attributes;
  }
  CopyMem (SmmVariableHeader->Name, VariableName, SmmVariableHeader->NameSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);

  //
  // Get data from SMM.
  //
  if (Status == EFI_SUCCESS || Status == EFI_BUFFER_TOO_SMALL) {
    //
    // SMM CommBuffer DataSize can be a trimed value
    // Only update DataSize when needed
    //
    *DataSize = SmmVariableHeader->DataSize;
  }
  if (Attributes != NULL) {
    *Attributes = SmmVariableHeader->Attributes;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  CopyMem (Data, (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize, SmmVariableHeader->DataSize);

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}


/**
  This code Finds the Next available variable.

  @param[in, out] VariableNameSize   Size of the variable name.
  @param[in, out] VariableName       Pointer to variable name.
  @param[in, out] VendorGuid         Variable Vendor Guid.

  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_SUCCESS                Find the specified variable.
  @retval EFI_NOT_FOUND              Not found.
  @retval EFI_BUFFER_TO_SMALL        DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetNextVariableName (
  IN OUT  UINTN                             *VariableNameSize,
  IN OUT  CHAR16                            *VariableName,
  IN OUT  EFI_GUID                          *VendorGuid
  )
{
  EFI_STATUS                                      Status;
  UINTN                                           PayloadSize;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *SmmGetNextVariableName;
  UINTN                                           OutVariableNameSize;
  UINTN                                           InVariableNameSize;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OutVariableNameSize   = *VariableNameSize;
  InVariableNameSize    = StrSize (VariableName);

  //
  // If input string exceeds SMM payload limit. Return failure
  //
  if (InVariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  if (OutVariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
    //
    // If output buffer exceed SMM payload limit. Trim output buffer to SMM payload size
    //
    OutVariableNameSize = mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name);
  }
  //
  // Payload should be Guid + NameSize + MAX of Input & Output buffer
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name) + MAX (OutVariableNameSize, InVariableNameSize);


  Status = InitCommunicateBuffer ((VOID **)&SmmGetNextVariableName, PayloadSize, SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  ASSERT (SmmGetNextVariableName != NULL);

  //
  // SMM comm buffer->NameSize is buffer size for return string
  //
  SmmGetNextVariableName->NameSize = OutVariableNameSize;

  CopyGuid (&SmmGetNextVariableName->Guid, VendorGuid);
  //
  // Copy whole string
  //
  CopyMem (SmmGetNextVariableName->Name, VariableName, InVariableNameSize);
  if (OutVariableNameSize > InVariableNameSize) {
    ZeroMem ((UINT8 *) SmmGetNextVariableName->Name + InVariableNameSize, OutVariableNameSize - InVariableNameSize);
  }

  //
  // Send data to SMM
  //
  Status = SendCommunicateBuffer (PayloadSize);

  //
  // Get data from SMM.
  //
  if (Status == EFI_SUCCESS || Status == EFI_BUFFER_TOO_SMALL) {
    //
    // SMM CommBuffer NameSize can be a trimed value
    // Only update VariableNameSize when needed
    //
    *VariableNameSize = SmmGetNextVariableName->NameSize;
  }
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  CopyGuid (VendorGuid, &SmmGetNextVariableName->Guid);
  CopyMem (VariableName, SmmGetNextVariableName->Name, SmmGetNextVariableName->NameSize);  

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
  @param[in] Attributes                   Attribute value of the variable found
  @param[in] DataSize                     Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in] Data                         Data pointer.

  @retval EFI_INVALID_PARAMETER           Invalid parameter.
  @retval EFI_SUCCESS                     Set successfully.
  @retval EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @retval EFI_NOT_FOUND                   Not found.
  @retval EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
RuntimeServiceSetVariable (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid,
  IN UINT32                                 Attributes,
  IN UINTN                                  DataSize,
  IN VOID                                   *Data
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize; 
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *SmmVariableHeader;
  UINTN                                     VariableNameSize;
    
  //
  // Check input parameters.
  //
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  } 

  if (DataSize != 0 && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableNameSize      = StrSize (VariableName);

  //
  // If VariableName or DataSize exceeds SMM payload limit. Return failure
  //
  if ((VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
      (DataSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) - VariableNameSize)){
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableServicesLock);
 
  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + VariableNameSize + DataSize;
  Status = InitCommunicateBuffer ((VOID **)&SmmVariableHeader, PayloadSize, SMM_VARIABLE_FUNCTION_SET_VARIABLE);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  ASSERT (SmmVariableHeader != NULL);

  CopyGuid ((EFI_GUID *) &SmmVariableHeader->Guid, VendorGuid);
  SmmVariableHeader->DataSize   = DataSize;
  SmmVariableHeader->NameSize   = VariableNameSize;
  SmmVariableHeader->Attributes = Attributes;
  CopyMem (SmmVariableHeader->Name, VariableName, SmmVariableHeader->NameSize);
  CopyMem ((UINT8 *) SmmVariableHeader->Name + SmmVariableHeader->NameSize, Data, DataSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}


/**
  This code returns information about the EFI variables.

  @param[in]  Attributes                   Attributes bitmask to specify the type of variables
                                           on which to return information.
  @param[out] MaximumVariableStorageSize   Pointer to the maximum size of the storage space available
                                           for the EFI variables associated with the attributes specified.
  @param[out] RemainingVariableStorageSize Pointer to the remaining size of the storage space available
                                           for EFI variables associated with the attributes specified.
  @param[out] MaximumVariableSize          Pointer to the maximum size of an individual EFI variables
                                           associated with the attributes specified.

  @retval EFI_INVALID_PARAMETER            An invalid combination of attribute bits was supplied.
  @retval EFI_SUCCESS                      Query successfully.
  @retval EFI_UNSUPPORTED                  The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
RuntimeServiceQueryVariableInfo (
  IN  UINT32                                Attributes,
  OUT UINT64                                *MaximumVariableStorageSize,
  OUT UINT64                                *RemainingVariableStorageSize,
  OUT UINT64                                *MaximumVariableSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *SmmQueryVariableInfo;

  if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize;
  //
  PayloadSize = sizeof (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO);
  Status = InitCommunicateBuffer ((VOID **)&SmmQueryVariableInfo, PayloadSize, SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  ASSERT (SmmQueryVariableInfo != NULL);

  SmmQueryVariableInfo->Attributes  = Attributes;

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data from SMM.
  //
  *MaximumVariableSize          = SmmQueryVariableInfo->MaximumVariableSize;
  *MaximumVariableStorageSize   = SmmQueryVariableInfo->MaximumVariableStorageSize;
  *RemainingVariableStorageSize = SmmQueryVariableInfo->RemainingVariableStorageSize; 

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}


/**
  Exit Boot Services Event notification handler.

  Notify SMM variable driver about the event.

  @param[in]  Event     Event whose notification function is being invoked.
  @param[in]  Context   Pointer to the notification function's context.

**/
VOID
EFIAPI
OnExitBootServices (
  IN      EFI_EVENT                         Event,
  IN      VOID                              *Context
  )
{
  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE.
  //
  InitCommunicateBuffer (NULL, 0, SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE); 

  //
  // Send data to SMM.
  //
  SendCommunicateBuffer (0);
}


/**
  On Ready To Boot Services Event notification handler.

  Notify SMM variable driver about the event.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
OnReadyToBoot (
  IN      EFI_EVENT                         Event,
  IN      VOID                              *Context
  )
{
  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE.
  //
  InitCommunicateBuffer (NULL, 0, SMM_VARIABLE_FUNCTION_READY_TO_BOOT);
  
  //
  // Send data to SMM.
  //
  SendCommunicateBuffer (0);
}


/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableAddressChangeEvent (
  IN EFI_EVENT                              Event,
  IN VOID                                   *Context
  )
{
  EfiConvertPointer (0x0, (VOID **) &mVariableBuffer);
  EfiConvertPointer (0x0, (VOID **) &mSmmCommunication);
}


/**
  Initialize variable service and install Variable Architectural protocol.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
 
**/
VOID
EFIAPI
SmmVariableReady (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *Context
  )
{
  EFI_STATUS                                Status;

  Status = gBS->LocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&mSmmVariable);
  if (EFI_ERROR (Status)) {
    return;
  }
  
  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &mSmmCommunication);
  ASSERT_EFI_ERROR (Status);
  
  //
  // Allocate memory for variable communicate buffer.
  //
  mVariableBufferPayloadSize = MAX (PcdGet32 (PcdMaxVariableSize), PcdGet32 (PcdMaxHardwareErrorVariableSize)) +
                               OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) - sizeof (VARIABLE_HEADER);
  mVariableBufferSize  = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + mVariableBufferPayloadSize;
  mVariableBuffer      = AllocateRuntimePool (mVariableBufferSize);
  ASSERT (mVariableBuffer != NULL);

  //
  // Save the buffer physical address used for SMM conmunication.
  //
  mVariableBufferPhysical = mVariableBuffer;

  gRT->GetVariable         = RuntimeServiceGetVariable;
  gRT->GetNextVariableName = RuntimeServiceGetNextVariableName;
  gRT->SetVariable         = RuntimeServiceSetVariable;
  gRT->QueryVariableInfo   = RuntimeServiceQueryVariableInfo;
 
  //
  // Install the Variable Architectural Protocol on a new handle.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableArchProtocolGuid, 
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}


/**
  SMM Non-Volatile variable write service is ready notify event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
  
**/
VOID
EFIAPI
SmmVariableWriteReady (
  IN  EFI_EVENT                             Event,
  IN  VOID                                  *Context
  )
{
  EFI_STATUS                                Status;
  VOID                                      *ProtocolOps;

  //
  // Check whether the protocol is installed or not.
  //
  Status = gBS->LocateProtocol (&gSmmVariableWriteGuid, NULL, (VOID **) &ProtocolOps);
  if (EFI_ERROR (Status)) {
    return;
  }
 
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableWriteArchProtocolGuid, 
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);  
}


/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols 
  for variable read and write services being available. It also registers
  a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableSmmRuntimeInitialize (
  IN EFI_HANDLE                             ImageHandle,
  IN EFI_SYSTEM_TABLE                       *SystemTable
  )
{
  VOID                                      *SmmVariableRegistration;
  VOID                                      *SmmVariableWriteRegistration;
  EFI_EVENT                                 OnReadyToBootEvent;
  EFI_EVENT                                 ExitBootServiceEvent;

  EfiInitializeLock (&mVariableServicesLock, TPL_NOTIFY);

  //
  // Smm variable service is ready
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiSmmVariableProtocolGuid, 
    TPL_CALLBACK, 
    SmmVariableReady, 
    NULL, 
    &SmmVariableRegistration
    );

  //
  // Smm Non-Volatile variable write service is ready
  //
  EfiCreateProtocolNotifyEvent (
    &gSmmVariableWriteGuid, 
    TPL_CALLBACK, 
    SmmVariableWriteReady, 
    NULL, 
    &SmmVariableWriteRegistration
    );

  //
  // Register the event to reclaim variable for OS usage.
  //
  EfiCreateEventReadyToBootEx (
    TPL_NOTIFY, 
    OnReadyToBoot, 
    NULL, 
    &OnReadyToBootEvent
    );             

  //
  // Register the event to inform SMM variable that it is at runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnExitBootServices,
         NULL,
         &gEfiEventExitBootServicesGuid,
         &ExitBootServiceEvent
         ); 

  //
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         VariableAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );
  
  return EFI_SUCCESS;
}

