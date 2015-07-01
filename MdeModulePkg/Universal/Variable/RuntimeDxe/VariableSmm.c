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

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
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
#include <Protocol/SmmEndOfDxe.h>
#include <Protocol/SmmVarCheck.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/SmmMemLib.h>

#include <Guid/SmmVariableCommon.h>
#include "Variable.h"

extern VARIABLE_INFO_ENTRY                           *gVariableInfo;
EFI_HANDLE                                           mSmmVariableHandle      = NULL;
EFI_HANDLE                                           mVariableHandle         = NULL;
BOOLEAN                                              mAtRuntime              = FALSE;
EFI_GUID                                             mZeroGuid               = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
UINT8                                                *mVariableBufferPayload = NULL;
UINTN                                                mVariableBufferPayloadSize;
extern BOOLEAN                                       mEndOfDxe;
extern BOOLEAN                                       mEnableLocking;

/**
  SecureBoot Hook for SetVariable.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.

**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16                                 *VariableName,
  IN EFI_GUID                               *VendorGuid
  )
{
  return ;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
SmmVariableSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
{
  EFI_STATUS                 Status;

  //
  // Disable write protection when the calling SetVariable() through EFI_SMM_VARIABLE_PROTOCOL.
  //
  mEnableLocking = FALSE;
  Status         = VariableServiceSetVariable (
                     VariableName,
                     VendorGuid,
                     Attributes,
                     DataSize,
                     Data
                     );
  mEnableLocking = TRUE;
  return Status;
}

EFI_SMM_VARIABLE_PROTOCOL      gSmmVariable = {
  VariableServiceGetVariable,
  VariableServiceGetNextVariableName,
  SmmVariableSetVariable,
  VariableServiceQueryVariableInfo
};

EDKII_SMM_VAR_CHECK_PROTOCOL mSmmVarCheck = { VarCheckRegisterSetVariableCheckHandler,
                                              VarCheckVariablePropertySet,
                                              VarCheckVariablePropertyGet };

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
    FreePool (*Buffer);
    *Buffer = NULL;
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
  EFI_GUID                                             VendorGuid;

  if (InfoEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableInfo = gVariableInfo;
  if (VariableInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  StatisticsInfoSize = sizeof (VARIABLE_INFO_ENTRY) + StrSize (VariableInfo->Name);
  if (*InfoSize < StatisticsInfoSize) {
    *InfoSize = StatisticsInfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }
  InfoName = (CHAR16 *)(InfoEntry + 1);

  CopyGuid (&VendorGuid, &InfoEntry->VendorGuid);

  if (CompareGuid (&VendorGuid, &mZeroGuid)) {
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
    if (CompareGuid (&VariableInfo->VendorGuid, &VendorGuid)) {
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
  SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE        *GetPayloadSize;
  VARIABLE_INFO_ENTRY                              *VariableInfo;
  SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE           *VariableToLock;
  SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *CommVariableProperty;
  UINTN                                            InfoSize;
  UINTN                                            NameBufferSize;
  UINTN                                            CommBufferPayloadSize;
  UINTN                                            TempCommBufferSize;

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize < SMM_VARIABLE_COMMUNICATE_HEADER_SIZE) {
    DEBUG ((EFI_D_ERROR, "SmmVariableHandler: SMM communication buffer size invalid!\n"));
    return EFI_SUCCESS;
  }
  CommBufferPayloadSize = TempCommBufferSize - SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  if (CommBufferPayloadSize > mVariableBufferPayloadSize) {
    DEBUG ((EFI_D_ERROR, "SmmVariableHandler: SMM communication buffer payload size invalid!\n"));
    return EFI_SUCCESS;
  }

  if (!SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((EFI_D_ERROR, "SmmVariableHandler: SMM communication buffer in SMRAM or overflow!\n"));
    return EFI_SUCCESS;
  }

  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)CommBuffer;
  switch (SmmVariableFunctionHeader->Function) {
    case SMM_VARIABLE_FUNCTION_GET_VARIABLE:
      if (CommBufferPayloadSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
        DEBUG ((EFI_D_ERROR, "GetVariable: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *) mVariableBufferPayload;
      if (((UINTN)(~0) - SmmVariableHeader->DataSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
         ((UINTN)(~0) - SmmVariableHeader->NameSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + SmmVariableHeader->DataSize)) {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }
      InfoSize = OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)
                 + SmmVariableHeader->DataSize + SmmVariableHeader->NameSize;

      //
      // SMRAM range check already covered before
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((EFI_D_ERROR, "GetVariable: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (SmmVariableHeader->NameSize < sizeof (CHAR16) || SmmVariableHeader->Name[SmmVariableHeader->NameSize/sizeof (CHAR16) - 1] != L'\0') {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VariableServiceGetVariable (
                 SmmVariableHeader->Name,
                 &SmmVariableHeader->Guid,
                 &SmmVariableHeader->Attributes,
                 &SmmVariableHeader->DataSize,
                 (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize
                 );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    case SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME:
      if (CommBufferPayloadSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
        DEBUG ((EFI_D_ERROR, "GetNextVariableName: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      GetNextVariableName = (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *) mVariableBufferPayload;
      if ((UINTN)(~0) - GetNextVariableName->NameSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }
      InfoSize = OFFSET_OF(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name) + GetNextVariableName->NameSize;

      //
      // SMRAM range check already covered before
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((EFI_D_ERROR, "GetNextVariableName: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      NameBufferSize = CommBufferPayloadSize - OFFSET_OF(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name);
      if (NameBufferSize < sizeof (CHAR16) || GetNextVariableName->Name[NameBufferSize/sizeof (CHAR16) - 1] != L'\0') {
        //
        // Make sure input VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VariableServiceGetNextVariableName (
                 &GetNextVariableName->NameSize,
                 GetNextVariableName->Name,
                 &GetNextVariableName->Guid
                 );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    case SMM_VARIABLE_FUNCTION_SET_VARIABLE:
      if (CommBufferPayloadSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
        DEBUG ((EFI_D_ERROR, "SetVariable: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *) mVariableBufferPayload;
      if (((UINTN)(~0) - SmmVariableHeader->DataSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
         ((UINTN)(~0) - SmmVariableHeader->NameSize < OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + SmmVariableHeader->DataSize)) {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }
      InfoSize = OFFSET_OF(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)
                 + SmmVariableHeader->DataSize + SmmVariableHeader->NameSize;

      //
      // SMRAM range check already covered before
      // Data buffer should not contain SMM range
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((EFI_D_ERROR, "SetVariable: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (SmmVariableHeader->NameSize < sizeof (CHAR16) || SmmVariableHeader->Name[SmmVariableHeader->NameSize/sizeof (CHAR16) - 1] != L'\0') {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VariableServiceSetVariable (
                 SmmVariableHeader->Name,
                 &SmmVariableHeader->Guid,
                 SmmVariableHeader->Attributes,
                 SmmVariableHeader->DataSize,
                 (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize
                 );
      break;

    case SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO)) {
        DEBUG ((EFI_D_ERROR, "QueryVariableInfo: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      QueryVariableInfo = (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *) SmmVariableFunctionHeader->Data;

      Status = VariableServiceQueryVariableInfo (
                 QueryVariableInfo->Attributes,
                 &QueryVariableInfo->MaximumVariableStorageSize,
                 &QueryVariableInfo->RemainingVariableStorageSize,
                 &QueryVariableInfo->MaximumVariableSize
                 );
      break;

    case SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE)) {
        DEBUG ((EFI_D_ERROR, "GetPayloadSize: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      GetPayloadSize = (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *) SmmVariableFunctionHeader->Data;
      GetPayloadSize->VariablePayloadSize = mVariableBufferPayloadSize;
      Status = EFI_SUCCESS;
      break;

    case SMM_VARIABLE_FUNCTION_READY_TO_BOOT:
      mEndOfDxe = TRUE;
      //
      // The initialization for variable quota.
      //
      InitializeVariableQuota ();
      if (AtRuntime()) {
        Status = EFI_UNSUPPORTED;
        break;
      }
      ReclaimForOS ();
      Status = EFI_SUCCESS;
      break;

    case SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE:
      mAtRuntime = TRUE;
      Status = EFI_SUCCESS;
      break;

    case SMM_VARIABLE_FUNCTION_GET_STATISTICS:
      VariableInfo = (VARIABLE_INFO_ENTRY *) SmmVariableFunctionHeader->Data;
      InfoSize = TempCommBufferSize - SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;

      //
      // Do not need to check SmmVariableFunctionHeader->Data in SMRAM here.
      // It is covered by previous CommBuffer check
      //

      if (!SmmIsBufferOutsideSmmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)CommBufferSize, sizeof(UINTN))) {
        DEBUG ((EFI_D_ERROR, "GetStatistics: SMM communication buffer in SMRAM!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = SmmVariableGetStatistics (VariableInfo, &InfoSize);
      *CommBufferSize = InfoSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
      break;

    case SMM_VARIABLE_FUNCTION_LOCK_VARIABLE:
      if (mEndOfDxe) {
        Status = EFI_ACCESS_DENIED;
      } else {
        VariableToLock = (SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE *) SmmVariableFunctionHeader->Data;
        Status = VariableLockRequestToLock (
                   NULL,
                   VariableToLock->Name,
                   &VariableToLock->Guid
                   );
      }
      break;
    case SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET:
      if (mEndOfDxe) {
        Status = EFI_ACCESS_DENIED;
      } else {
        CommVariableProperty = (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *) SmmVariableFunctionHeader->Data;
        Status = VarCheckVariablePropertySet (
                   CommVariableProperty->Name,
                   &CommVariableProperty->Guid,
                   &CommVariableProperty->VariableProperty
                   );
      }
      break;
    case SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
        DEBUG ((EFI_D_ERROR, "VarCheckVariablePropertyGet: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }
      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      CommVariableProperty = (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *) mVariableBufferPayload;
      if ((UINTN) (~0) - CommVariableProperty->NameSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }
      InfoSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name) + CommVariableProperty->NameSize;

      //
      // SMRAM range check already covered before
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((EFI_D_ERROR, "VarCheckVariablePropertyGet: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (CommVariableProperty->NameSize < sizeof (CHAR16) || CommVariableProperty->Name[CommVariableProperty->NameSize/sizeof (CHAR16) - 1] != L'\0') {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VarCheckVariablePropertyGet (
                 CommVariableProperty->Name,
                 &CommVariableProperty->Guid,
                 &CommVariableProperty->VariableProperty
                 );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    default:
      Status = EFI_UNSUPPORTED;
  }

EXIT:

  SmmVariableFunctionHeader->ReturnStatus = Status;

  return EFI_SUCCESS;
}

/**
  SMM END_OF_DXE protocol notification event handler.

  @param  Protocol   Points to the protocol's unique identifier
  @param  Interface  Points to the interface instance
  @param  Handle     The handle on which the interface was installed

  @retval EFI_SUCCESS   SmmEndOfDxeCallback runs successfully

**/
EFI_STATUS
EFIAPI
SmmEndOfDxeCallback (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  DEBUG ((EFI_D_INFO, "[Variable]END_OF_DXE is signaled\n"));
  mEndOfDxe = TRUE;
  //
  // The initialization for variable quota.
  //
  InitializeVariableQuota ();
  if (PcdGetBool (PcdReclaimVariableSpaceAtEndOfDxe)) {
    ReclaimForOS ();
  }
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
  UINTN                                   FtwMaxBlockSize;

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

  Status = FtwProtocol->GetMaxBlockSize (FtwProtocol, &FtwMaxBlockSize);
  if (!EFI_ERROR (Status)) {
    ASSERT (PcdGet32 (PcdFlashNvStorageVariableSize) <= FtwMaxBlockSize);
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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Variable write service initialization failed. Status = %r\n", Status));
  }

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
  VOID                                    *SmmEndOfDxeRegistration;

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

  Status = gSmst->SmmInstallProtocolInterface (
                    &VariableHandle,
                    &gEdkiiSmmVarCheckProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmVarCheck
                    );
  ASSERT_EFI_ERROR (Status);

  mVariableBufferPayloadSize = GetNonVolatileMaxVariableSize () +
                               OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name) - GetVariableHeaderSize ();

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    mVariableBufferPayloadSize,
                    (VOID **)&mVariableBufferPayload
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
  // Register EFI_SMM_END_OF_DXE_PROTOCOL_GUID notify function.
  //
  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmEndOfDxeProtocolGuid,
                    SmmEndOfDxeCallback,
                    &SmmEndOfDxeRegistration
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


