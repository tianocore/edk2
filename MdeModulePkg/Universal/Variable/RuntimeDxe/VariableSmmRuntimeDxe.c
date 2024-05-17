/** @file
  Implement all four UEFI Runtime Variable services for the nonvolatile
  and volatile storage space and install variable architecture protocol
  based on SMM variable module.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  RuntimeServiceGetVariable() and RuntimeServiceSetVariable() are external API
  to receive data buffer. The size should be checked carefully.

  InitCommunicateBuffer() is really function to check the variable data size.

Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Variable.h>
#include <Protocol/MmCommunication2.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/VariableLock.h>
#include <Protocol/VarCheck.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>

#include <Guid/EventGroup.h>
#include <Guid/SmmVariableCommon.h>
#include <Guid/VariableRuntimeCacheInfo.h>

#include "PrivilegePolymorphic.h"
#include "VariableParsing.h"

EFI_HANDLE                      mHandle                    = NULL;
EFI_SMM_VARIABLE_PROTOCOL       *mSmmVariable              = NULL;
EFI_EVENT                       mVirtualAddressChangeEvent = NULL;
EFI_MM_COMMUNICATION2_PROTOCOL  *mMmCommunication2         = NULL;
UINT8                           *mVariableBuffer           = NULL;
UINT8                           *mVariableBufferPhysical   = NULL;
VARIABLE_INFO_ENTRY             *mVariableInfo             = NULL;
UINTN                           mVariableBufferSize;
UINTN                           mVariableBufferPayloadSize;
BOOLEAN                         mVariableAuthFormat;
EFI_LOCK                        mVariableServicesLock;
EDKII_VARIABLE_LOCK_PROTOCOL    mVariableLock;
EDKII_VAR_CHECK_PROTOCOL        mVarCheck;
VARIABLE_RUNTIME_CACHE_INFO     mVariableRtCacheInfo;
BOOLEAN                         mIsRuntimeCacheEnabled = FALSE;

/**
  The logic to initialize the VariablePolicy engine is in its own file.

**/
EFI_STATUS
EFIAPI
VariablePolicySmmDxeMain (
  IN    EFI_HANDLE        ImageHandle,
  IN    EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Some Secure Boot Policy Variable may update following other variable changes(SecureBoot follows PK change, etc).
  Record their initial State when variable write service is ready.

**/
VOID
EFIAPI
RecordSecureBootPolicyVarData (
  VOID
  );

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
  IN EFI_LOCK  *Lock
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
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called. FALSE if ExitBootServices () has not been called.
**/
BOOLEAN
AtRuntime (
  VOID
  )
{
  return EfiAtRuntime ();
}

/**
  Initialize the communicate buffer using DataSize and Function.

  The communicate size is: SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE +
  DataSize.

  Caution: This function may receive untrusted input.
  The data size external input, so this function will validate it carefully to avoid buffer overflow.

  @param[out]      DataPtr          Points to the data in the communicate buffer.
  @param[in]       DataSize         The data size to send to SMM.
  @param[in]       Function         The function number to initialize the communicate header.

  @retval EFI_INVALID_PARAMETER     The data size is too big.
  @retval EFI_SUCCESS               Find the specified variable.

**/
EFI_STATUS
InitCommunicateBuffer (
  OUT     VOID   **DataPtr OPTIONAL,
  IN      UINTN  DataSize,
  IN      UINTN  Function
  )
{
  EFI_MM_COMMUNICATE_HEADER        *SmmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER  *SmmVariableFunctionHeader;

  if (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE > mVariableBufferSize) {
    return EFI_INVALID_PARAMETER;
  }

  SmmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mVariableBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;

  SmmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
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
  IN      UINTN  DataSize
  )
{
  EFI_STATUS                       Status;
  UINTN                            CommSize;
  EFI_MM_COMMUNICATE_HEADER        *SmmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER  *SmmVariableFunctionHeader;

  CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  Status   = mMmCommunication2->Communicate (
                                  mMmCommunication2,
                                  mVariableBufferPhysical,
                                  mVariableBuffer,
                                  &CommSize
                                  );
  ASSERT_EFI_ERROR (Status);

  SmmCommunicateHeader      = (EFI_MM_COMMUNICATE_HEADER *)mVariableBuffer;
  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
  return SmmVariableFunctionHeader->ReturnStatus;
}

/**
  Mark a variable that will become read-only after leaving the DXE phase of execution.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL  *This,
  IN       CHAR16                        *VariableName,
  IN       EFI_GUID                      *VendorGuid
  )
{
  EFI_STATUS                              Status;
  UINTN                                   VariableNameSize;
  UINTN                                   PayloadSize;
  SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE  *VariableToLock;

  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VariableNameSize = StrSize (VariableName);
  VariableToLock   = NULL;

  //
  // If VariableName exceeds SMM payload limit. Return failure
  //
  if (VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE, Name)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE, Name) + VariableNameSize;
  Status      = InitCommunicateBuffer ((VOID **)&VariableToLock, PayloadSize, SMM_VARIABLE_FUNCTION_LOCK_VARIABLE);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ASSERT (VariableToLock != NULL);

  CopyGuid (&VariableToLock->Guid, VendorGuid);
  VariableToLock->NameSize = VariableNameSize;
  CopyMem (VariableToLock->Name, VariableName, VariableToLock->NameSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  Register SetVariable check handler.

  @param[in] Handler            Pointer to check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
EFI_STATUS
EFIAPI
VarCheckRegisterSetVariableCheckHandler (
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER  Handler
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Variable property set.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertySet (
  IN CHAR16                       *Name,
  IN EFI_GUID                     *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  EFI_STATUS                                            Status;
  UINTN                                                 VariableNameSize;
  UINTN                                                 PayloadSize;
  SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY  *CommVariableProperty;

  if ((Name == NULL) || (Name[0] == 0) || (Guid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty->Revision != VAR_CHECK_VARIABLE_PROPERTY_REVISION) {
    return EFI_INVALID_PARAMETER;
  }

  VariableNameSize     = StrSize (Name);
  CommVariableProperty = NULL;

  //
  // If VariableName exceeds SMM payload limit. Return failure
  //
  if (VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name) + VariableNameSize;
  Status      = InitCommunicateBuffer ((VOID **)&CommVariableProperty, PayloadSize, SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ASSERT (CommVariableProperty != NULL);

  CopyGuid (&CommVariableProperty->Guid, Guid);
  CopyMem (&CommVariableProperty->VariableProperty, VariableProperty, sizeof (*VariableProperty));
  CommVariableProperty->NameSize = VariableNameSize;
  CopyMem (CommVariableProperty->Name, Name, CommVariableProperty->NameSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
EFI_STATUS
EFIAPI
VarCheckVariablePropertyGet (
  IN CHAR16                        *Name,
  IN EFI_GUID                      *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY  *VariableProperty
  )
{
  EFI_STATUS                                            Status;
  UINTN                                                 VariableNameSize;
  UINTN                                                 PayloadSize;
  SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY  *CommVariableProperty;

  if ((Name == NULL) || (Name[0] == 0) || (Guid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableProperty == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableNameSize     = StrSize (Name);
  CommVariableProperty = NULL;

  //
  // If VariableName exceeds SMM payload limit. Return failure
  //
  if (VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name) + VariableNameSize;
  Status      = InitCommunicateBuffer ((VOID **)&CommVariableProperty, PayloadSize, SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ASSERT (CommVariableProperty != NULL);

  CopyGuid (&CommVariableProperty->Guid, Guid);
  CommVariableProperty->NameSize = VariableNameSize;
  CopyMem (CommVariableProperty->Name, Name, CommVariableProperty->NameSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);
  if (Status == EFI_SUCCESS) {
    CopyMem (VariableProperty, &CommVariableProperty->VariableProperty, sizeof (*VariableProperty));
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  Signals SMM to synchronize any pending variable updates with the runtime cache(s).

**/
VOID
SyncRuntimeCache (
  VOID
  )
{
  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE.
  //
  InitCommunicateBuffer (NULL, 0, SMM_VARIABLE_FUNCTION_SYNC_RUNTIME_CACHE);

  //
  // Send data to SMM.
  //
  SendCommunicateBuffer (0);
}

/**
  Check whether a SMI must be triggered to retrieve pending cache updates.

  If the variable HOB was finished being flushed since the last check for a runtime cache update, this function
  will prevent the HOB cache from being used for future runtime cache hits.

**/
VOID
CheckForRuntimeCacheSync (
  VOID
  )
{
  CACHE_INFO_FLAG  *CacheInfoFlag;

  CacheInfoFlag = (CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer;

  if (CacheInfoFlag->PendingUpdate) {
    SyncRuntimeCache ();
  }

  ASSERT (!(CacheInfoFlag->PendingUpdate));

  //
  // The HOB variable data may have finished being flushed in the runtime cache sync update
  //
  if ((CacheInfoFlag->HobFlushComplete) && (mVariableRtCacheInfo.RuntimeHobCacheBuffer != 0)) {
    mVariableRtCacheInfo.RuntimeHobCacheBuffer = 0;
  }
}

/**
  Finds the given variable in a runtime cache variable store.

  Caution: This function may receive untrusted input.
  The data size is external input, so this function will validate it carefully to avoid buffer overflow.

  @param[in]      VariableName       Name of Variable to be found.
  @param[in]      VendorGuid         Variable vendor GUID.
  @param[out]     Attributes         Attribute value of the variable found.
  @param[in, out] DataSize           Size of Data found. If size is less than the
                                     data, this value contains the required size.
  @param[out]     Data               Data pointer.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.

**/
EFI_STATUS
FindVariableInRuntimeCache (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  EFI_STATUS              Status;
  UINTN                   TempDataSize;
  VARIABLE_POINTER_TRACK  RtPtrTrack;
  VARIABLE_STORE_TYPE     StoreType;
  VARIABLE_STORE_HEADER   *VariableStoreList[VariableStoreTypeMax];
  CACHE_INFO_FLAG         *CacheInfoFlag;

  Status        = EFI_NOT_FOUND;
  CacheInfoFlag = (CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&RtPtrTrack, sizeof (RtPtrTrack));

  //
  // The UEFI specification restricts Runtime Services callers from invoking the same or certain other Runtime Service
  // functions prior to completion and return from a previous Runtime Service call. These restrictions prevent
  // a GetVariable () or GetNextVariable () call from being issued until a prior call has returned. The runtime
  // cache read lock should always be free when entering this function.
  //
  ASSERT (!(CacheInfoFlag->ReadLock));

  CacheInfoFlag->ReadLock = TRUE;
  CheckForRuntimeCacheSync ();

  if (!(CacheInfoFlag->PendingUpdate)) {
    //
    // 0: Volatile, 1: HOB, 2: Non-Volatile.
    // The index and attributes mapping must be kept in this order as FindVariable
    // makes use of this mapping to implement search algorithm.
    //
    VariableStoreList[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeVolatileCacheBuffer;
    VariableStoreList[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeHobCacheBuffer;
    VariableStoreList[VariableStoreTypeNv]       = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeNvCacheBuffer;

    for (StoreType = (VARIABLE_STORE_TYPE)0; StoreType < VariableStoreTypeMax; StoreType++) {
      if (VariableStoreList[StoreType] == NULL) {
        continue;
      }

      RtPtrTrack.StartPtr = GetStartPointer (VariableStoreList[StoreType]);
      RtPtrTrack.EndPtr   = GetEndPointer (VariableStoreList[StoreType]);
      RtPtrTrack.Volatile = (BOOLEAN)(StoreType == VariableStoreTypeVolatile);

      Status = FindVariableEx (VariableName, VendorGuid, FALSE, &RtPtrTrack, mVariableAuthFormat);
      if (!EFI_ERROR (Status)) {
        break;
      }
    }

    if (!EFI_ERROR (Status)) {
      //
      // Get data size
      //
      TempDataSize = DataSizeOfVariable (RtPtrTrack.CurrPtr, mVariableAuthFormat);
      ASSERT (TempDataSize != 0);

      if (*DataSize >= TempDataSize) {
        if (Data == NULL) {
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }

        CopyMem (Data, GetVariableDataPtr (RtPtrTrack.CurrPtr, mVariableAuthFormat), TempDataSize);
        *DataSize = TempDataSize;

        UpdateVariableInfo (VariableName, VendorGuid, RtPtrTrack.Volatile, TRUE, FALSE, FALSE, TRUE, &mVariableInfo);

        Status = EFI_SUCCESS;
        goto Done;
      } else {
        *DataSize = TempDataSize;
        Status    = EFI_BUFFER_TOO_SMALL;
        goto Done;
      }
    }
  }

Done:
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
    if ((Attributes != NULL) && (RtPtrTrack.CurrPtr != NULL)) {
      *Attributes = RtPtrTrack.CurrPtr->Attributes;
    }
  }

  CacheInfoFlag->ReadLock = FALSE;

  return Status;
}

/**
  Finds the given variable in a variable store in SMM.

  Caution: This function may receive untrusted input.
  The data size is external input, so this function will validate it carefully to avoid buffer overflow.

  @param[in]      VariableName       Name of Variable to be found.
  @param[in]      VendorGuid         Variable vendor GUID.
  @param[out]     Attributes         Attribute value of the variable found.
  @param[in, out] DataSize           Size of Data found. If size is less than the
                                     data, this value contains the required size.
  @param[out]     Data               Data pointer.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.

**/
EFI_STATUS
FindVariableInSmm (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *SmmVariableHeader;
  UINTN                                     TempDataSize;
  UINTN                                     VariableNameSize;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TempDataSize      = *DataSize;
  VariableNameSize  = StrSize (VariableName);
  SmmVariableHeader = NULL;

  //
  // If VariableName exceeds SMM payload limit. Return failure
  //
  if (VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
    return EFI_INVALID_PARAMETER;
  }

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
  SmmVariableHeader->DataSize = TempDataSize;
  SmmVariableHeader->NameSize = VariableNameSize;
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
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
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

  if (Data != NULL) {
    CopyMem (Data, (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize, SmmVariableHeader->DataSize);
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

Done:
  return Status;
}

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  The data size is external input, so this function will validate it carefully to avoid buffer overflow.

  @param[in]      VariableName       Name of Variable to be found.
  @param[in]      VendorGuid         Variable vendor GUID.
  @param[out]     Attributes         Attribute value of the variable found.
  @param[in, out] DataSize           Size of Data found. If size is less than the
                                     data, this value contains the required size.
  @param[out]     Data               Data pointer.

  @retval EFI_SUCCESS                The function completed successfully.
  @retval EFI_NOT_FOUND              The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL       The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER      VariableName is NULL.
  @retval EFI_INVALID_PARAMETER      VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER      DataSize is NULL.
  @retval EFI_INVALID_PARAMETER      The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION     The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED            After ExitBootServices() has been called, this return code may be returned
                                     if no variable storage is supported. The platform should describe this
                                     runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                     configuration table.

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  )
{
  EFI_STATUS  Status;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);
  if (mIsRuntimeCacheEnabled) {
    Status = FindVariableInRuntimeCache (VariableName, VendorGuid, Attributes, DataSize, Data);
  } else {
    Status = FindVariableInSmm (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);

  return Status;
}

/**
  Finds the next available variable in a runtime cache variable store.

  @param[in, out] VariableNameSize   Size of the variable name.
  @param[in, out] VariableName       Pointer to variable name.
  @param[in, out] VendorGuid         Variable Vendor Guid.

  @retval EFI_SUCCESS                The function completed successfully.
  @retval EFI_NOT_FOUND              The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL       The VariableNameSize is too small for the result.
                                     VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER      VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER      VariableName is NULL.
  @retval EFI_INVALID_PARAMETER      VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER      The input values of VariableName and VendorGuid are not a name and
                                     GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER      Null-terminator is not found in the first VariableNameSize bytes of
                                     the input VariableName buffer.
  @retval EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED            After ExitBootServices() has been called, this return code may be returned
                                     if no variable storage is supported. The platform should describe this
                                     runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                     configuration table.

**/
EFI_STATUS
GetNextVariableNameInRuntimeCache (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS             Status;
  UINTN                  VarNameSize;
  VARIABLE_HEADER        *VariablePtr;
  VARIABLE_STORE_HEADER  *VariableStoreHeader[VariableStoreTypeMax];
  CACHE_INFO_FLAG        *CacheInfoFlag;

  Status        = EFI_NOT_FOUND;
  CacheInfoFlag = (CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer;

  //
  // The UEFI specification restricts Runtime Services callers from invoking the same or certain other Runtime Service
  // functions prior to completion and return from a previous Runtime Service call. These restrictions prevent
  // a GetVariable () or GetNextVariable () call from being issued until a prior call has returned. The runtime
  // cache read lock should always be free when entering this function.
  //
  ASSERT (!(CacheInfoFlag->ReadLock));

  CheckForRuntimeCacheSync ();

  CacheInfoFlag->ReadLock = TRUE;
  if (!(CacheInfoFlag->PendingUpdate)) {
    //
    // 0: Volatile, 1: HOB, 2: Non-Volatile.
    // The index and attributes mapping must be kept in this order as FindVariable
    // makes use of this mapping to implement search algorithm.
    //
    VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeVolatileCacheBuffer;
    VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeHobCacheBuffer;
    VariableStoreHeader[VariableStoreTypeNv]       = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeNvCacheBuffer;

    Status =  VariableServiceGetNextVariableInternal (
                VariableName,
                VendorGuid,
                VariableStoreHeader,
                &VariablePtr,
                mVariableAuthFormat
                );
    if (!EFI_ERROR (Status)) {
      VarNameSize = NameSizeOfVariable (VariablePtr, mVariableAuthFormat);
      ASSERT (VarNameSize != 0);
      if (VarNameSize <= *VariableNameSize) {
        CopyMem (VariableName, GetVariableNamePtr (VariablePtr, mVariableAuthFormat), VarNameSize);
        CopyMem (VendorGuid, GetVendorGuidPtr (VariablePtr, mVariableAuthFormat), sizeof (EFI_GUID));
        Status = EFI_SUCCESS;
      } else {
        Status = EFI_BUFFER_TOO_SMALL;
      }

      *VariableNameSize = VarNameSize;
    }
  }

  CacheInfoFlag->ReadLock = FALSE;

  return Status;
}

/**
  Finds the next available variable in a SMM variable store.

  @param[in, out] VariableNameSize   Size of the variable name.
  @param[in, out] VariableName       Pointer to variable name.
  @param[in, out] VendorGuid         Variable Vendor Guid.

  @retval EFI_SUCCESS                The function completed successfully.
  @retval EFI_NOT_FOUND              The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL       The VariableNameSize is too small for the result.
                                     VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER      VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER      VariableName is NULL.
  @retval EFI_INVALID_PARAMETER      VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER      The input values of VariableName and VendorGuid are not a name and
                                     GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER      Null-terminator is not found in the first VariableNameSize bytes of
                                     the input VariableName buffer.
  @retval EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED            After ExitBootServices() has been called, this return code may be returned
                                     if no variable storage is supported. The platform should describe this
                                     runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                     configuration table.

**/
EFI_STATUS
GetNextVariableNameInSmm (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS                                       Status;
  UINTN                                            PayloadSize;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME  *SmmGetNextVariableName;
  UINTN                                            OutVariableNameSize;
  UINTN                                            InVariableNameSize;

  OutVariableNameSize    = *VariableNameSize;
  InVariableNameSize     = StrSize (VariableName);
  SmmGetNextVariableName = NULL;

  //
  // If input string exceeds SMM payload limit. Return failure
  //
  if (InVariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
    return EFI_INVALID_PARAMETER;
  }

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
    ZeroMem ((UINT8 *)SmmGetNextVariableName->Name + InVariableNameSize, OutVariableNameSize - InVariableNameSize);
  }

  //
  // Send data to SMM
  //
  Status = SendCommunicateBuffer (PayloadSize);

  //
  // Get data from SMM.
  //
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
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
  return Status;
}

/**
  This code Finds the Next available variable.

  @param[in, out] VariableNameSize   Size of the variable name.
  @param[in, out] VariableName       Pointer to variable name.
  @param[in, out] VendorGuid         Variable Vendor Guid.

  @retval EFI_SUCCESS                The function completed successfully.
  @retval EFI_NOT_FOUND              The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL       The VariableNameSize is too small for the result.
                                     VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER      VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER      VariableName is NULL.
  @retval EFI_INVALID_PARAMETER      VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER      The input values of VariableName and VendorGuid are not a name and
                                     GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER      Null-terminator is not found in the first VariableNameSize bytes of
                                     the input VariableName buffer.
  @retval EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED            After ExitBootServices() has been called, this return code may be returned
                                     if no variable storage is supported. The platform should describe this
                                     runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                     configuration table.

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;
  UINTN       MaxLen;

  Status = EFI_NOT_FOUND;

  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the possible maximum length of name string, including the Null terminator.
  //
  MaxLen = *VariableNameSize / sizeof (CHAR16);
  if ((MaxLen == 0) || (StrnLenS (VariableName, MaxLen) == MaxLen)) {
    //
    // Null-terminator is not found in the first VariableNameSize bytes of the input VariableName buffer,
    // follow spec to return EFI_INVALID_PARAMETER.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);
  if (mIsRuntimeCacheEnabled) {
    Status = GetNextVariableNameInRuntimeCache (VariableNameSize, VariableName, VendorGuid);
  } else {
    Status = GetNextVariableNameInSmm (VariableNameSize, VariableName, VendorGuid);
  }

  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);

  return Status;
}

/**
  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  The data size and data are external input, so this function will validate it carefully to avoid buffer overflow.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
  @param[in] Attributes                   Attribute value of the variable found
  @param[in] DataSize                     Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in] Data                         Data pointer.

  @retval EFI_SUCCESS                     The firmware has successfully stored the variable and its data as
                                          defined by the Attributes.
  @retval EFI_INVALID_PARAMETER           An invalid combination of attribute bits, name, and GUID was supplied, or the
                                          DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER           VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES            Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR                The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED             The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED             The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION          The variable could not be written due to
                                          EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set,
                                          but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND                   The variable trying to be updated or deleted was not found.
  @retval EFI_UNSUPPORTED                 This call is not supported by this platform at the time the call is made.
                                          The platform should describe this runtime service as unsupported at runtime
                                          via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
EFI_STATUS
EFIAPI
RuntimeServiceSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_STATUS                                Status;
  UINTN                                     PayloadSize;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *SmmVariableHeader;
  UINTN                                     VariableNameSize;

  //
  // Check input parameters.
  //
  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VariableNameSize  = StrSize (VariableName);
  SmmVariableHeader = NULL;

  //
  // If VariableName or DataSize exceeds SMM payload limit. Return failure
  //
  if ((VariableNameSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
      (DataSize > mVariableBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) - VariableNameSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize.
  //
  PayloadSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + VariableNameSize + DataSize;
  Status      = InitCommunicateBuffer ((VOID **)&SmmVariableHeader, PayloadSize, SMM_VARIABLE_FUNCTION_SET_VARIABLE);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ASSERT (SmmVariableHeader != NULL);

  CopyGuid ((EFI_GUID *)&SmmVariableHeader->Guid, VendorGuid);
  SmmVariableHeader->DataSize   = DataSize;
  SmmVariableHeader->NameSize   = VariableNameSize;
  SmmVariableHeader->Attributes = Attributes;
  CopyMem (SmmVariableHeader->Name, VariableName, SmmVariableHeader->NameSize);
  CopyMem ((UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize, Data, DataSize);

  //
  // Send data to SMM.
  //
  Status = SendCommunicateBuffer (PayloadSize);

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);

  if (!EfiAtRuntime ()) {
    if (!EFI_ERROR (Status)) {
      SecureBootHook (
        VariableName,
        VendorGuid
        );
    }
  }

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
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         PayloadSize;
  SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO  *SmmQueryVariableInfo;

  SmmQueryVariableInfo = NULL;

  if ((MaximumVariableStorageSize == NULL) || (RemainingVariableStorageSize == NULL) || (MaximumVariableSize == NULL) || (Attributes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + PayloadSize;
  //
  PayloadSize = sizeof (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO);
  Status      = InitCommunicateBuffer ((VOID **)&SmmQueryVariableInfo, PayloadSize, SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ASSERT (SmmQueryVariableInfo != NULL);

  SmmQueryVariableInfo->Attributes = Attributes;

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
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
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
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
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

  //
  // Install the system configuration table for variable info data captured
  //
  if (mIsRuntimeCacheEnabled && FeaturePcdGet (PcdVariableCollectStatistics)) {
    if (mVariableAuthFormat) {
      gBS->InstallConfigurationTable (&gEfiAuthenticatedVariableGuid, mVariableInfo);
    } else {
      gBS->InstallConfigurationTable (&gEfiVariableGuid, mVariableInfo);
    }
  }

  gBS->CloseEvent (Event);
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
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mVariableBuffer);
  EfiConvertPointer (0x0, (VOID **)&mMmCommunication2);
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mVariableRtCacheInfo.CacheInfoFlagBuffer);
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mVariableRtCacheInfo.RuntimeHobCacheBuffer);
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mVariableRtCacheInfo.RuntimeNvCacheBuffer);
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mVariableRtCacheInfo.RuntimeVolatileCacheBuffer);
}

/**
  This code gets variable payload size.

  @param[out] VariablePayloadSize   Output pointer to variable payload size.

  @retval EFI_SUCCESS               Get successfully.
  @retval Others                    Get unsuccessfully.

**/
EFI_STATUS
EFIAPI
GetVariablePayloadSize (
  OUT UINTN  *VariablePayloadSize
  )
{
  EFI_STATUS                                 Status;
  SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE  *SmmGetPayloadSize;
  EFI_MM_COMMUNICATE_HEADER                  *SmmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER            *SmmVariableFunctionHeader;
  UINTN                                      CommSize;
  UINT8                                      *CommBuffer;

  SmmGetPayloadSize = NULL;
  CommBuffer        = NULL;

  if (VariablePayloadSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE);
  //
  CommSize   = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE);
  CommBuffer = AllocateZeroPool (CommSize);
  if (CommBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  SmmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE);

  SmmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
  SmmVariableFunctionHeader->Function = SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE;
  SmmGetPayloadSize                   = (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *)SmmVariableFunctionHeader->Data;

  //
  // Send data to SMM.
  //
  Status = mMmCommunication2->Communicate (mMmCommunication2, CommBuffer, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  Status = SmmVariableFunctionHeader->ReturnStatus;
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data from SMM.
  //
  *VariablePayloadSize = SmmGetPayloadSize->VariablePayloadSize;

Done:
  if (CommBuffer != NULL) {
    FreePool (CommBuffer);
  }

  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  This code gets information needed from SMM for runtime cache initialization.

  @param[out] TotalHobStorageSize         Output pointer for the total HOB storage size in bytes.
  @param[out] TotalNvStorageSize          Output pointer for the total non-volatile storage size in bytes.
  @param[out] TotalVolatileStorageSize    Output pointer for the total volatile storage size in bytes.
  @param[out] AuthenticatedVariableUsage  Output pointer that indicates if authenticated variables are to be used.

  @retval EFI_SUCCESS                     Retrieved the size successfully.
  @retval EFI_INVALID_PARAMETER           TotalNvStorageSize parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES            The memory resources needed for a CommBuffer are not available.
  @retval Others                          Could not retrieve the size successfully.

**/
EFI_STATUS
GetRuntimeCacheInfo (
  OUT UINTN    *TotalHobStorageSize,
  OUT UINTN    *TotalNvStorageSize,
  OUT UINTN    *TotalVolatileStorageSize,
  OUT BOOLEAN  *AuthenticatedVariableUsage
  )
{
  EFI_STATUS                                       Status;
  SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO  *SmmGetRuntimeCacheInfo;
  EFI_MM_COMMUNICATE_HEADER                        *SmmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER                  *SmmVariableFunctionHeader;
  UINTN                                            CommSize;
  UINT8                                            *CommBuffer;

  SmmGetRuntimeCacheInfo = NULL;
  CommBuffer             = mVariableBuffer;

  if ((TotalHobStorageSize == NULL) || (TotalNvStorageSize == NULL) || (TotalVolatileStorageSize == NULL) || (AuthenticatedVariableUsage == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CommBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  CommSize = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO);
  ZeroMem (CommBuffer, CommSize);

  SmmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO);

  SmmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
  SmmVariableFunctionHeader->Function = SMM_VARIABLE_FUNCTION_GET_RUNTIME_CACHE_INFO;
  SmmGetRuntimeCacheInfo              = (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO *)SmmVariableFunctionHeader->Data;

  //
  // Send data to SMM.
  //
  Status = mMmCommunication2->Communicate (mMmCommunication2, CommBuffer, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);
  if (CommSize <= SMM_VARIABLE_COMMUNICATE_HEADER_SIZE) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  Status = SmmVariableFunctionHeader->ReturnStatus;
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data from SMM.
  //
  *TotalHobStorageSize        = SmmGetRuntimeCacheInfo->TotalHobStorageSize;
  *TotalNvStorageSize         = SmmGetRuntimeCacheInfo->TotalNvStorageSize;
  *TotalVolatileStorageSize   = SmmGetRuntimeCacheInfo->TotalVolatileStorageSize;
  *AuthenticatedVariableUsage = SmmGetRuntimeCacheInfo->AuthenticatedVariableUsage;

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  Initialize the variable cache buffer as an empty variable store.

  @param[in]  VariableCacheBuffer     A pointer to pointer of a cache variable store.
  @param[in]  TotalVariableCacheSize  The size needed for the UEFI variable store cache buffer that is allocated.

**/
VOID
InitVariableStoreHeader (
  IN  VARIABLE_STORE_HEADER  *VariableCacheBuffer,
  IN  UINTN                  TotalVariableCacheSize
  )
{
  if (TotalVariableCacheSize > 0) {
    ASSERT ((VariableCacheBuffer != NULL) && (TotalVariableCacheSize >= sizeof (VARIABLE_STORE_HEADER)));

    SetMem32 ((VOID *)VariableCacheBuffer, TotalVariableCacheSize, (UINT32)0xFFFFFFFF);
    ZeroMem ((VOID *)VariableCacheBuffer, sizeof (VARIABLE_STORE_HEADER));
    VariableCacheBuffer->Size   = (UINT32)TotalVariableCacheSize;
    VariableCacheBuffer->Format = VARIABLE_STORE_FORMATTED;
    VariableCacheBuffer->State  = VARIABLE_STORE_HEALTHY;
  }
}

/**
  Initialize the runtime variable cache related content.

  @param[in]  RuntimeCacheInfoGuidHob     Pointer to the gEdkiiVariableRuntimeCacheInfoHobGuid HOB.

  @retval EFI_SUCCESS    Initialize the runtime variable cache related content successfully.
  @retval Others         Could not initialize the runtime variable cache related content successfully.

**/
EFI_STATUS
InitVariableCache (
  IN  EFI_HOB_GUID_TYPE  *RuntimeCacheInfoGuidHob
  )
{
  EFI_STATUS                   Status;
  UINTN                        ExpectedHobCacheSize;
  UINTN                        ExpectedNvCacheSize;
  UINTN                        ExpectedVolatileCacheSize;
  UINTN                        AllocatedHobCacheSize;
  UINTN                        AllocatedNvCacheSize;
  UINTN                        AllocatedVolatileCacheSize;
  VARIABLE_RUNTIME_CACHE_INFO  *VariableRuntimeCacheInfo;

  //
  // Get needed runtime cache buffer size and check if auth variables are to be used from SMM
  //
  Status =  GetRuntimeCacheInfo (
              &ExpectedHobCacheSize,
              &ExpectedNvCacheSize,
              &ExpectedVolatileCacheSize,
              &mVariableAuthFormat
              );
  if (!EFI_ERROR (Status)) {
    VariableRuntimeCacheInfo   = GET_GUID_HOB_DATA (RuntimeCacheInfoGuidHob);
    AllocatedHobCacheSize      = EFI_PAGES_TO_SIZE ((UINTN)VariableRuntimeCacheInfo->RuntimeHobCachePages);
    AllocatedNvCacheSize       = EFI_PAGES_TO_SIZE ((UINTN)VariableRuntimeCacheInfo->RuntimeNvCachePages);
    AllocatedVolatileCacheSize = EFI_PAGES_TO_SIZE ((UINTN)VariableRuntimeCacheInfo->RuntimeVolatileCachePages);

    ASSERT (
      (AllocatedHobCacheSize >= ExpectedHobCacheSize) &&
      (AllocatedNvCacheSize >= ExpectedNvCacheSize) &&
      (AllocatedVolatileCacheSize >= ExpectedVolatileCacheSize)
      );

    CopyMem (&mVariableRtCacheInfo, VariableRuntimeCacheInfo, sizeof (VARIABLE_RUNTIME_CACHE_INFO));
    InitVariableStoreHeader ((VOID *)(UINTN)mVariableRtCacheInfo.RuntimeHobCacheBuffer, AllocatedHobCacheSize);
    InitVariableStoreHeader ((VOID *)(UINTN)mVariableRtCacheInfo.RuntimeNvCacheBuffer, AllocatedNvCacheSize);
    InitVariableStoreHeader ((VOID *)(UINTN)mVariableRtCacheInfo.RuntimeVolatileCacheBuffer, AllocatedVolatileCacheSize);
  }

  return Status;
}

/**
  Sends the runtime variable cache context information to SMM.

  @retval EFI_SUCCESS               Retrieved the size successfully.
  @retval EFI_INVALID_PARAMETER     TotalNvStorageSize parameter is NULL.
  @retval EFI_OUT_OF_RESOURCES      The memory resources needed for a CommBuffer are not available.
  @retval Others                    Could not retrieve the size successfully.;

**/
EFI_STATUS
SendRuntimeVariableCacheContextToSmm (
  VOID
  )
{
  EFI_STATUS                                               Status;
  SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT  *SmmRuntimeVarCacheContext;
  EFI_MM_COMMUNICATE_HEADER                                *SmmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER                          *SmmVariableFunctionHeader;
  UINTN                                                    CommSize;
  UINT8                                                    *CommBuffer;

  SmmRuntimeVarCacheContext = NULL;
  CommBuffer                = mVariableBuffer;

  if (CommBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AcquireLockOnlyAtBootTime (&mVariableServicesLock);

  //
  // Init the communicate buffer. The buffer data size is:
  // SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT);
  //
  CommSize = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT);
  ZeroMem (CommBuffer, CommSize);

  SmmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommBuffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  SmmCommunicateHeader->MessageLength = SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + sizeof (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT);

  SmmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;
  SmmVariableFunctionHeader->Function = SMM_VARIABLE_FUNCTION_INIT_RUNTIME_VARIABLE_CACHE_CONTEXT;
  SmmRuntimeVarCacheContext           = (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT *)SmmVariableFunctionHeader->Data;

  SmmRuntimeVarCacheContext->RuntimeHobCache      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeHobCacheBuffer;
  SmmRuntimeVarCacheContext->RuntimeVolatileCache = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeVolatileCacheBuffer;
  SmmRuntimeVarCacheContext->RuntimeNvCache       = (VARIABLE_STORE_HEADER *)(UINTN)mVariableRtCacheInfo.RuntimeNvCacheBuffer;
  SmmRuntimeVarCacheContext->PendingUpdate        = &((CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer)->PendingUpdate;
  SmmRuntimeVarCacheContext->ReadLock             = &((CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer)->ReadLock;
  SmmRuntimeVarCacheContext->HobFlushComplete     = &((CACHE_INFO_FLAG *)(UINTN)mVariableRtCacheInfo.CacheInfoFlagBuffer)->HobFlushComplete;

  //
  // Send data to SMM.
  //
  Status = mMmCommunication2->Communicate (mMmCommunication2, CommBuffer, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);
  if (CommSize <= SMM_VARIABLE_COMMUNICATE_HEADER_SIZE) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  Status = SmmVariableFunctionHeader->ReturnStatus;
  if (EFI_ERROR (Status)) {
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&mVariableServicesLock);
  return Status;
}

/**
  Initialize variable service and install Variable Architectural protocol.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmVariableReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS         Status;
  EFI_HOB_GUID_TYPE  *GuidHob;

  Status = gBS->LocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&mSmmVariable);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiMmCommunication2ProtocolGuid, NULL, (VOID **)&mMmCommunication2);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate memory for variable communicate buffer.
  //
  Status = GetVariablePayloadSize (&mVariableBufferPayloadSize);
  ASSERT_EFI_ERROR (Status);
  mVariableBufferSize = SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE + mVariableBufferPayloadSize;
  mVariableBuffer     = AllocateRuntimePool (mVariableBufferSize);
  ASSERT (mVariableBuffer != NULL);

  //
  // Save the buffer physical address used for SMM conmunication.
  //
  mVariableBufferPhysical = mVariableBuffer;

  GuidHob = GetFirstGuidHob (&gEdkiiVariableRuntimeCacheInfoHobGuid);
  if (GuidHob != NULL) {
    mIsRuntimeCacheEnabled = TRUE;
    DEBUG ((DEBUG_INFO, "Variable driver runtime cache is enabled.\n"));

    Status = InitVariableCache (GuidHob);
    if (!EFI_ERROR (Status)) {
      Status = SendRuntimeVariableCacheContextToSmm ();
      if (!EFI_ERROR (Status)) {
        SyncRuntimeCache ();
      }
    }

    if (EFI_ERROR (Status)) {
      ZeroMem (&mVariableRtCacheInfo, sizeof (VARIABLE_RUNTIME_CACHE_INFO));
    }

    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((DEBUG_INFO, "Variable driver runtime cache is disabled.\n"));
  }

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

  mVariableLock.RequestToLock = VariableLockRequestToLock;
  Status                      = gBS->InstallMultipleProtocolInterfaces (
                                       &mHandle,
                                       &gEdkiiVariableLockProtocolGuid,
                                       &mVariableLock,
                                       NULL
                                       );
  ASSERT_EFI_ERROR (Status);

  mVarCheck.RegisterSetVariableCheckHandler = VarCheckRegisterSetVariableCheckHandler;
  mVarCheck.VariablePropertySet             = VarCheckVariablePropertySet;
  mVarCheck.VariablePropertyGet             = VarCheckVariablePropertyGet;
  Status                                    = gBS->InstallMultipleProtocolInterfaces (
                                                     &mHandle,
                                                     &gEdkiiVarCheckProtocolGuid,
                                                     &mVarCheck,
                                                     NULL
                                                     );
  ASSERT_EFI_ERROR (Status);

  gBS->CloseEvent (Event);
}

/**
  SMM Non-Volatile variable write service is ready notify event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
SmmVariableWriteReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *ProtocolOps;

  //
  // Check whether the protocol is installed or not.
  //
  Status = gBS->LocateProtocol (&gSmmVariableWriteGuid, NULL, (VOID **)&ProtocolOps);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Some Secure Boot Policy Var (SecureBoot, etc) updates following other
  // Secure Boot Policy Variable change.  Record their initial value.
  //
  RecordSecureBootPolicyVarData ();

  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiVariableWriteArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  gBS->CloseEvent (Event);
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID       *SmmVariableRegistration;
  VOID       *SmmVariableWriteRegistration;
  EFI_EVENT  OnReadyToBootEvent;
  EFI_EVENT  ExitBootServiceEvent;
  EFI_EVENT  LegacyBootEvent;

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
  // Register the event to inform SMM variable that it is at runtime for legacy boot.
  // Reuse OnExitBootServices() here.
  //
  EfiCreateEventLegacyBootEx (
    TPL_NOTIFY,
    OnExitBootServices,
    NULL,
    &LegacyBootEvent
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

  // Initialize the VariablePolicy protocol and engine.
  VariablePolicySmmDxeMain (ImageHandle, SystemTable);

  return EFI_SUCCESS;
}
