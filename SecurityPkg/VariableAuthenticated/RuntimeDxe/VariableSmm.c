/** @file
  The sample implementation for SMM variable protocol. And this driver 
  implements an SMI handler to communicate with the DXE runtime driver 
  to provide variable services.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data and communicate buffer in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  SmmVariableHandler() will receive untrusted input and do basic validation.

  Each sub function VariableServiceGetVariable(), VariableServiceGetNextVariableName(), 
  VariableServiceSetVariable(), VariableServiceQueryVariableInfo(), ReclaimForOS(), 
  SmmVariableGetStatistics() should also do validation based on its own knowledge.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Protocol/SmmVariable.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Protocol/SmmFaultTolerantWrite.h>
#include <Library/SmmServicesTableLib.h>

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/SmmVariableCommon.h>
#include "Variable.h"

extern VARIABLE_INFO_ENTRY                           *gVariableInfo;
EFI_HANDLE                                           mSmmVariableHandle      = NULL;
EFI_HANDLE                                           mVariableHandle         = NULL;
BOOLEAN                                              mAtRuntime              = FALSE;
EFI_GUID                                             mZeroGuid               = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
  
EFI_SMM_VARIABLE_PROTOCOL      gSmmVariable = {
  VariableServiceGetVariable,
  VariableServiceGetNextVariableName,
  VariableServiceSetVariable,
  VariableServiceQueryVariableInfo
};


/**
  Return TRUE if ExitBootServices () has been called.
  
  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  )
{
  return mAtRuntime;
}

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state 
  and returns the lock.  Each lock provides mutual exclusion access at its task 
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK                         *Lock,
  IN EFI_TPL                              Priority
  )
{
  return Lock;
}

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

}

/**
  Retrive the SMM Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of SMM Ftw protocol

  @retval EFI_SUCCESS           The SMM FTW protocol instance was found and returned in FtwProtocol.
  @retval EFI_NOT_FOUND         The SMM FTW protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID                                **FtwProtocol
  )
{
  EFI_STATUS                              Status;

  //
  // Locate Smm Fault Tolerent Write protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmFaultTolerantWriteProtocolGuid, 
                    NULL, 
                    FtwProtocol
                    );
  return Status;
}


/**
  Retrive the SMM FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of SMM FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of SMM FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the SMM FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.

**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  )
{
  //
  // To get the SMM FVB protocol interface on the handle
  //
  return gSmst->SmmHandleProtocol (
                  FvBlockHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  (VOID **) FvBlock
                  );
}


/**
  Function returns an array of handles that support the SMM FVB protocol
  in a buffer allocated from pool. 

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support SMM FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No SMM FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.

**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN                               *NumberHandles,
  OUT EFI_HANDLE                          **Buffer
  )
{
  EFI_STATUS                              Status;
  UINTN                                   BufferSize;

  if ((NumberHandles == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status = gSmst->SmmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );
  if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gSmst->SmmLocateHandle (
                    ByProtocol,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    NULL,
                    &BufferSize,
                    *Buffer
                    );

  *NumberHandles = BufferSize / sizeof(EFI_HANDLE);
  if (EFI_ERROR(Status)) {
    *NumberHandles = 0;
  }

  return Status;
}


/**
  Get the variable statistics information from the information buffer pointed by gVariableInfo.

  Caution: This function may be invoked at SMM runtime.
  InfoEntry and InfoSize are external input. Care must be taken to make sure not security issue at runtime.

  @param[in, out]  InfoEntry    A pointer to the buffer of variable information entry.
                                On input, point to the variable information returned last time. if 
                                InfoEntry->VendorGuid is zero, return the first information.
                                On output, point to the next variable information.
  @param[in, out]  InfoSize     On input, the size of the variable information buffer.
                                On output, the returned variable information size.

  @retval EFI_SUCCESS           The variable information is found and returned successfully.
  @retval EFI_UNSUPPORTED       No variable inoformation exists in variable driver. The 
                                PcdVariableCollectStatistics should be set TRUE to support it.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is too small to hold the next variable information.
  @retval EFI_INVALID_PARAMETER Input parameter is invalid.

**/
EFI_STATUS
SmmVariableGetStatistics (
  IN OUT VARIABLE_INFO_ENTRY                           *InfoEntry,
  IN OUT UINTN                                         *InfoSize
  )
{
  VARIABLE_INFO_ENTRY                                  *VariableInfo;
  UINTN                                                NameLength;
  UINTN                                                StatisticsInfoSize;
  CHAR16                                               *InfoName;
 
  if (InfoEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  VariableInfo = gVariableInfo; 
  if (VariableInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY) + StrSize (VariableInfo->Name);
  if (*InfoSize < sizeof (VARIABLE_INFO_ENTRY)) {
    *InfoSize = StatisticsInfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }
  InfoName = (CHAR16 *)(InfoEntry + 1);

  if (CompareGuid (&InfoEntry->VendorGuid, &mZeroGuid)) {
    //
    // Return the first variable info
    //
    CopyMem (InfoEntry, VariableInfo, sizeof (VARIABLE_INFO_ENTRY));
    CopyMem (InfoName, VariableInfo->Name, StrSize (VariableInfo->Name));
    *InfoSize = StatisticsInfoSize;
    return EFI_SUCCESS;
  }

  //
  // Get the next variable info
  //
  while (VariableInfo != NULL) {
    if (CompareGuid (&VariableInfo->VendorGuid, &InfoEntry->VendorGuid)) {
      NameLength = StrSize (VariableInfo->Name);
      if (NameLength == StrSize (InfoName)) {
        if (CompareMem (VariableInfo->Name, InfoName, NameLength) == 0) {
          //
          // Find the match one
          //
          VariableInfo = VariableInfo->Next;
          break;
        }
      }
    }
    VariableInfo = VariableInfo->Next;
  };
    
  if (VariableInfo == NULL) {
    *InfoSize = 0;
    return EFI_SUCCESS;
  }

  //
  // Output the new variable info
  //
  StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY) + StrSize (VariableInfo->Name);
  if (*InfoSize < StatisticsInfoSize) {
    *InfoSize = StatisticsInfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (InfoEntry, VariableInfo, sizeof (VARIABLE_INFO_ENTRY));
  CopyMem (InfoName, VariableInfo->Name, StrSize (VariableInfo->Name));
  *InfoSize = StatisticsInfoSize;
  
  return EFI_SUCCESS;
}


/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the variable wrapper driver.

  Caution: This function may receive untrusted input.
  This variable data and communicate buffer are external input, so this function will do basic validation.
  Each sub function VariableServiceGetVariable(), VariableServiceGetNextVariableName(), 
  VariableServiceSetVariable(), VariableServiceQueryVariableInfo(), ReclaimForOS(), 
  SmmVariableGetStatistics() should also do validation based on its own knowledge.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
  @retval EFI_INVALID_PARAMETER               Input parameter is invalid.

**/
EFI_STATUS
EFIAPI
SmmVariableHandler (
  IN     EFI_HANDLE                                DispatchHandle,
  IN     CONST VOID                                *RegisterContext,
  IN OUT VOID                                      *CommBuffer,
  IN OUT UINTN                                     *CommBufferSize
  )
{
  EFI_STATUS                                       Status;
  SMM_VARIABLE_COMMUNICATE_HEADER                  *SmmVariableFunctionHeader;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE         *SmmVariableHeader;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME  *GetNextVariableName;
  SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO     *QueryVariableInfo;
  VARIABLE_INFO_ENTRY                              *VariableInfo;
  UINTN                                            InfoSize;

  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)CommBuffer;
  switch (SmmVariableFunctionHeader->Function) {
    case SMM_VARIABLE_FUNCTION_GET_VARIABLE:
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *) SmmVariableFunctionHeader->Data;     
      Status = VariableServiceGetVariable (
                 SmmVariableHeader->Name,
                 &SmmVariableHeader->Guid,
                 &SmmVariableHeader->Attributes,
                 &SmmVariableHeader->DataSize,
                 (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize
                 );
      break;
      
    case SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME:
      GetNextVariableName = (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *) SmmVariableFunctionHeader->Data;
      Status = VariableServiceGetNextVariableName (
                 &GetNextVariableName->NameSize,
                 GetNextVariableName->Name,
                 &GetNextVariableName->Guid
                 );
      break;
      
    case SMM_VARIABLE_FUNCTION_SET_VARIABLE:
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *) SmmVariableFunctionHeader->Data;
      Status = VariableServiceSetVariable (
                 SmmVariableHeader->Name,
                 &SmmVariableHeader->Guid,
                 SmmVariableHeader->Attributes,
                 SmmVariableHeader->DataSize,
                 (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize
                 );
      break;
      
    case SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO:
      QueryVariableInfo = (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *) SmmVariableFunctionHeader->Data;
      Status = VariableServiceQueryVariableInfo (
                 QueryVariableInfo->Attributes,
                 &QueryVariableInfo->MaximumVariableStorageSize,
                 &QueryVariableInfo->RemainingVariableStorageSize,
                 &QueryVariableInfo->MaximumVariableSize
                 );
      break;

    case SMM_VARIABLE_FUNCTION_READY_TO_BOOT:
      ReclaimForOS ();
      Status = EFI_SUCCESS;
      break;
  
    case SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE:
      mAtRuntime = TRUE;
      Status = EFI_SUCCESS;
      break;

    case SMM_VARIABLE_FUNCTION_GET_STATISTICS:
      VariableInfo = (VARIABLE_INFO_ENTRY *) SmmVariableFunctionHeader->Data;
      InfoSize = *CommBufferSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_HEADER, Data);
      Status = SmmVariableGetStatistics (VariableInfo, &InfoSize);
      *CommBufferSize = InfoSize + OFFSET_OF (SMM_VARIABLE_COMMUNICATE_HEADER, Data);
      break;

    default:
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
  }

  SmmVariableFunctionHeader->ReturnStatus = Status;

  return EFI_SUCCESS;
}


/**
  SMM Fault Tolerant Write protocol notification event handler.

  Non-Volatile variable write may needs FTW protocol to reclaim when 
  writting variable.
  
  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEventCallback runs successfully
  @retval EFI_NOT_FOUND The Fvb protocol for variable is not found.
  
 **/
EFI_STATUS
EFIAPI
SmmFtwNotificationEvent (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  EFI_STATUS                              Status;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_SMM_FAULT_TOLERANT_WRITE_PROTOCOL   *FtwProtocol;
  EFI_PHYSICAL_ADDRESS                    NvStorageVariableBase;
  
  if (mVariableModuleGlobal->FvbInstance != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Ensure SMM FTW protocol is installed.
  //
  Status = GetFtwProtocol ((VOID **)&FtwProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find the proper FVB protocol for variable.
  //
  NvStorageVariableBase = (EFI_PHYSICAL_ADDRESS) PcdGet64 (PcdFlashNvStorageVariableBase64);
  if (NvStorageVariableBase == 0) {
    NvStorageVariableBase = (EFI_PHYSICAL_ADDRESS) PcdGet32 (PcdFlashNvStorageVariableBase);
  }
  Status = GetFvbInfoByAddress (NvStorageVariableBase, NULL, &FvbProtocol);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  mVariableModuleGlobal->FvbInstance = FvbProtocol;
  
  Status = VariableWriteServiceInitialize ();
  ASSERT_EFI_ERROR (Status);
 
  //
  // Notify the variable wrapper driver the variable write service is ready
  //
  Status = gBS->InstallProtocolInterface (
                  &mSmmVariableHandle,
                  &gSmmVariableWriteGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
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
VariableServiceInitialize (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                              Status;
  EFI_HANDLE                              VariableHandle;
  VOID                                    *SmmFtwRegistration;
  
  //
  // Variable initialize.
  //
  Status = VariableCommonInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Install the Smm Variable Protocol on a new handle.
  //
  VariableHandle = NULL;
  Status = gSmst->SmmInstallProtocolInterface (
                    &VariableHandle,
                    &gEfiSmmVariableProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gSmmVariable
                    );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Register SMM variable SMI handler
  ///
  VariableHandle = NULL;
  Status = gSmst->SmiHandlerRegister (SmmVariableHandler, &gEfiSmmVariableProtocolGuid, &VariableHandle);
  ASSERT_EFI_ERROR (Status);
  
  //
  // Notify the variable wrapper driver the variable service is ready
  //
  Status = SystemTable->BootServices->InstallProtocolInterface (
                                        &mVariableHandle,
                                        &gEfiSmmVariableProtocolGuid,
                                        EFI_NATIVE_INTERFACE,
                                        &gSmmVariable
                                        );
  ASSERT_EFI_ERROR (Status);
 
  //
  // Register FtwNotificationEvent () notify function.
  // 
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmFaultTolerantWriteProtocolGuid,
                    SmmFtwNotificationEvent,
                    &SmmFtwRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  SmmFtwNotificationEvent (NULL, NULL, NULL);
  
  return EFI_SUCCESS;
}


