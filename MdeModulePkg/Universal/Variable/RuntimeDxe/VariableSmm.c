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

Copyright (c) 2010 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Protocol/SmmVariable.h>
#include <Protocol/VariableStorage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>

#include <Guid/SmmVariableCommon.h>
#include "PrivilegePolymorphic.h"

extern VARIABLE_STORE_HEADER  *mNvVariableCache;

STATIC BOOLEAN  mAtRuntime              = FALSE;
STATIC BOOLEAN  mEndOfDxe               = FALSE;
STATIC UINT8    *mVariableBufferPayload = NULL;
STATIC UINTN    mVariableBufferPayloadSize;

STATIC EDKII_VARIABLE_STORAGE_PROTOCOL  *mVariableStorage;

/**
  This code gets the size of variable header.

  @return Size of variable header in bytes in type UINTN.

**/
STATIC
UINTN
EFIAPI
GetVariableHeaderSize (
  VOID
  )
{
  UINTN    Value;
  BOOLEAN  AuthFormat;

  AuthFormat = mVariableStorage->CheckAuthFormat ();

  if (AuthFormat) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
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
STATIC
EFI_STATUS
EFIAPI
VariableStorageFvbSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  return mVariableStorage->SetVariable (
                             VariableName,
                             VendorGuid,
                             Attributes,
                             DataSize,
                             Data,
                             TRUE
                             );
}

/**
  Returns the value of a variable.

  @param[in]       VariableName  A Null-terminated string that is the name of the vendor's
                                 variable.
  @param[in]       VendorGuid    A unique identifier for the vendor.
  @param[out]      Attributes    If not NULL, a pointer to the memory location to return the
                                 attributes bitmask for the variable.
  @param[in, out]  DataSize      On input, the size in bytes of the return Data buffer.
                                 On output the size of data returned in Data.
  @param[out]      Data          The buffer to return the contents of the variable. May be NULL
                                 with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED        After ExitBootServices() has been called, this return code may be returned
                                 if no variable storage is supported. The platform should describe this
                                 runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                 configuration table.

**/
STATIC
EFI_STATUS
EFIAPI
VariableStorageFvbGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes     OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data           OPTIONAL
  )
{
  return mVariableStorage->GetVariable (
                             VariableName,
                             VendorGuid,
                             Attributes,
                             DataSize,
                             Data
                             );
}

/**
  Enumerates the current variable names.

  @param[in, out]  VariableNameSize The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param[in, out]  VariableName     On input, supplies the last VariableName that was returned
                                    by GetNextVariableName(). On output, returns the Nullterminated
                                    string of the current variable.
  @param[in, out]  VendorGuid       On input, supplies the last VendorGuid that was returned by
                                    GetNextVariableName(). On output, returns the
                                    VendorGuid of the current variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the result.
                                VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER VariableName is NULL.
  @retval EFI_INVALID_PARAMETER VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER The input values of VariableName and VendorGuid are not a name and
                                GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER Null-terminator is not found in the first VariableNameSize bytes of
                                the input VariableName buffer.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED       After ExitBootServices() has been called, this return code may be returned
                                if no variable storage is supported. The platform should describe this
                                runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                configuration table.

**/
STATIC
EFI_STATUS
EFIAPI
VariableStorageFvbGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  return mVariableStorage->GetNextVariableName (
                             VariableNameSize,
                             VariableName,
                             VendorGuid
                             );
}

/**
  Returns information about the EFI variables.

  @param[in]   Attributes                   Attributes bitmask to specify the type of variables on
                                            which to return information.
  @param[out]  MaximumVariableStorageSize   On output the maximum size of the storage space
                                            available for the EFI variables associated with the
                                            attributes specified.
  @param[out]  RemainingVariableStorageSize Returns the remaining size of the storage space
                                            available for the EFI variables associated with the
                                            attributes specified.
  @param[out]  MaximumVariableSize          Returns the maximum size of the individual EFI
                                            variables associated with the attributes specified.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this platform, and the
                                       MaximumVariableStorageSize,
                                       RemainingVariableStorageSize, MaximumVariableSize
                                       are undefined.

**/
STATIC
EFI_STATUS
EFIAPI
VariableStorageFvbQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  return mVariableStorage->QueryVariableInfo (
                             Attributes,
                             MaximumVariableStorageSize,
                             RemainingVariableStorageSize,
                             MaximumVariableSize
                             );
}

EFI_SMM_VARIABLE_PROTOCOL  gSmmVariable = {
  VariableStorageFvbGetVariable,
  VariableStorageFvbGetNextVariableName,
  VariableStorageFvbSetVariable,
  VariableStorageFvbQueryVariableInfo
};

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
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  EFI_STATUS                                               Status;
  SMM_VARIABLE_COMMUNICATE_HEADER                          *SmmVariableFunctionHeader;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE                 *SmmVariableHeader;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME          *GetNextVariableName;
  SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO             *QueryVariableInfo;
  SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE                *GetPayloadSize;
  SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT  *RuntimeVariableCacheContext;
  SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO          *GetRuntimeCacheInfo;
  SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE                   *VariableToLock;
  SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY     *CommVariableProperty;
  VARIABLE_INFO_ENTRY                                      *VariableInfo;
  UINTN                                                    InfoSize;
  UINTN                                                    NameBufferSize;
  UINTN                                                    CommBufferPayloadSize;
  UINTN                                                    TempCommBufferSize;

  //
  // If input is invalid, stop processing this SMI
  //
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize < SMM_VARIABLE_COMMUNICATE_HEADER_SIZE) {
    DEBUG ((DEBUG_ERROR, "SmmVariableHandler: SMM communication buffer size invalid!\n"));
    return EFI_SUCCESS;
  }

  CommBufferPayloadSize = TempCommBufferSize - SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  if (CommBufferPayloadSize > mVariableBufferPayloadSize) {
    DEBUG ((DEBUG_ERROR, "SmmVariableHandler: SMM communication buffer payload size invalid!\n"));
    return EFI_SUCCESS;
  }

  if (!VariableSmmIsPrimaryBufferValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "SmmVariableHandler: SMM Primary Buffer (CommBuffer) is not valid!\n"));
    return EFI_SUCCESS;
  }

  SmmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)CommBuffer;
  switch (SmmVariableFunctionHeader->Function) {
    case SMM_VARIABLE_FUNCTION_GET_VARIABLE:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
        DEBUG ((DEBUG_ERROR, "GetVariable: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)mVariableBufferPayload;
      if (((UINTN)(~0) - SmmVariableHeader->DataSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
          ((UINTN)(~0) - SmmVariableHeader->NameSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + SmmVariableHeader->DataSize))
      {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      InfoSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)
                 + SmmVariableHeader->DataSize + SmmVariableHeader->NameSize;

      //
      // SMRAM range check already covered before
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((DEBUG_ERROR, "GetVariable: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // The VariableSpeculationBarrier() call here is to ensure the previous
      // range/content checks for the CommBuffer have been completed before the
      // subsequent consumption of the CommBuffer content.
      //
      VariableSpeculationBarrier ();
      if ((SmmVariableHeader->NameSize < sizeof (CHAR16)) || (SmmVariableHeader->Name[SmmVariableHeader->NameSize/sizeof (CHAR16) - 1] != L'\0')) {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VariableStorageFvbGetVariable (
                 SmmVariableHeader->Name,
                 &SmmVariableHeader->Guid,
                 &SmmVariableHeader->Attributes,
                 &SmmVariableHeader->DataSize,
                 (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize
                 );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    case SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
        DEBUG ((DEBUG_ERROR, "GetNextVariableName: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      GetNextVariableName = (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)mVariableBufferPayload;
      if ((UINTN)(~0) - GetNextVariableName->NameSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name)) {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      InfoSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name) + GetNextVariableName->NameSize;

      //
      // SMRAM range check already covered before
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((DEBUG_ERROR, "GetNextVariableName: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      NameBufferSize = CommBufferPayloadSize - OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name);
      if ((NameBufferSize < sizeof (CHAR16)) || (GetNextVariableName->Name[NameBufferSize/sizeof (CHAR16) - 1] != L'\0')) {
        //
        // Make sure input VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = VariableStorageFvbGetNextVariableName (
                 &GetNextVariableName->NameSize,
                 GetNextVariableName->Name,
                 &GetNextVariableName->Guid
                 );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    case SMM_VARIABLE_FUNCTION_SET_VARIABLE:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) {
        DEBUG ((DEBUG_ERROR, "SetVariable: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      SmmVariableHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)mVariableBufferPayload;
      if (((UINTN)(~0) - SmmVariableHeader->DataSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)) ||
          ((UINTN)(~0) - SmmVariableHeader->NameSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) + SmmVariableHeader->DataSize))
      {
        //
        // Prevent InfoSize overflow happen
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      InfoSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name)
                 + SmmVariableHeader->DataSize + SmmVariableHeader->NameSize;

      //
      // SMRAM range check already covered before
      // Data buffer should not contain SMM range
      //
      if (InfoSize > CommBufferPayloadSize) {
        DEBUG ((DEBUG_ERROR, "SetVariable: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // The VariableSpeculationBarrier() call here is to ensure the previous
      // range/content checks for the CommBuffer have been completed before the
      // subsequent consumption of the CommBuffer content.
      //
      VariableSpeculationBarrier ();
      if ((SmmVariableHeader->NameSize < sizeof (CHAR16)) || (SmmVariableHeader->Name[SmmVariableHeader->NameSize/sizeof (CHAR16) - 1] != L'\0')) {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = mVariableStorage->SetVariable (
                                   SmmVariableHeader->Name,
                                   &SmmVariableHeader->Guid,
                                   SmmVariableHeader->Attributes,
                                   SmmVariableHeader->DataSize,
                                   (UINT8 *)SmmVariableHeader->Name + SmmVariableHeader->NameSize,
                                   FALSE
                                   );
      break;

    case SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO)) {
        DEBUG ((DEBUG_ERROR, "QueryVariableInfo: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      QueryVariableInfo = (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *)SmmVariableFunctionHeader->Data;

      Status = VariableStorageFvbQueryVariableInfo (
                 QueryVariableInfo->Attributes,
                 &QueryVariableInfo->MaximumVariableStorageSize,
                 &QueryVariableInfo->RemainingVariableStorageSize,
                 &QueryVariableInfo->MaximumVariableSize
                 );
      break;

    case SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE)) {
        DEBUG ((DEBUG_ERROR, "GetPayloadSize: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      GetPayloadSize                      = (SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *)SmmVariableFunctionHeader->Data;
      GetPayloadSize->VariablePayloadSize = mVariableBufferPayloadSize;
      Status                              = EFI_SUCCESS;
      break;

    case SMM_VARIABLE_FUNCTION_READY_TO_BOOT:
      if (mAtRuntime) {
        Status = EFI_UNSUPPORTED;
        break;
      }

      if (!mEndOfDxe) {
        Status = mVariableStorage->ReadyToBoot ();
      } else {
        Status = EFI_SUCCESS;
      }

      break;

    case SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE:
      mAtRuntime = TRUE;
      Status     = mVariableStorage->ExitBootService ();
      break;

    case SMM_VARIABLE_FUNCTION_GET_STATISTICS:
      VariableInfo = (VARIABLE_INFO_ENTRY *)SmmVariableFunctionHeader->Data;
      InfoSize     = TempCommBufferSize - SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;

      //
      // Do not need to check SmmVariableFunctionHeader->Data in SMRAM here.
      // It is covered by previous CommBuffer check
      //

      //
      // Do not need to check CommBufferSize buffer as it should point to SMRAM
      // that was used by SMM core to cache CommSize from SmmCommunication protocol.
      //

      Status          = mVariableStorage->GetStatics (VariableInfo, &InfoSize);
      *CommBufferSize = InfoSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
      break;

    case SMM_VARIABLE_FUNCTION_LOCK_VARIABLE:
      if (mEndOfDxe) {
        Status = EFI_ACCESS_DENIED;
      } else {
        VariableToLock = (SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE *)SmmVariableFunctionHeader->Data;
        Status         = mVariableStorage->RequestToLock (
                                             VariableToLock->Name,
                                             &VariableToLock->Guid
                                             );
      }

      break;

    case SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET:
      if (mEndOfDxe) {
        Status = EFI_ACCESS_DENIED;
      } else {
        CommVariableProperty = (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)SmmVariableFunctionHeader->Data;
        Status               = mVariableStorage->PropertySet (
                                                   CommVariableProperty->Name,
                                                   &CommVariableProperty->Guid,
                                                   &CommVariableProperty->VariableProperty
                                                   );
      }

      break;

    case SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET:
      if (CommBufferPayloadSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
        DEBUG ((DEBUG_ERROR, "VarCheckVariablePropertyGet: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      //
      // Copy the input communicate buffer payload to pre-allocated SMM variable buffer payload.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      CommVariableProperty = (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)mVariableBufferPayload;
      if ((UINTN)(~0) - CommVariableProperty->NameSize < OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name)) {
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
        DEBUG ((DEBUG_ERROR, "VarCheckVariablePropertyGet: Data size exceed communication buffer size limit!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // The VariableSpeculationBarrier() call here is to ensure the previous
      // range/content checks for the CommBuffer have been completed before the
      // subsequent consumption of the CommBuffer content.
      //
      VariableSpeculationBarrier ();
      if ((CommVariableProperty->NameSize < sizeof (CHAR16)) || (CommVariableProperty->Name[CommVariableProperty->NameSize/sizeof (CHAR16) - 1] != L'\0')) {
        //
        // Make sure VariableName is A Null-terminated string.
        //
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = mVariableStorage->PropertyGet (
                                   CommVariableProperty->Name,
                                   &CommVariableProperty->Guid,
                                   &CommVariableProperty->VariableProperty
                                   );
      CopyMem (SmmVariableFunctionHeader->Data, mVariableBufferPayload, CommBufferPayloadSize);
      break;

    case SMM_VARIABLE_FUNCTION_INIT_RUNTIME_VARIABLE_CACHE_CONTEXT:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT)) {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: SMM communication buffer size invalid!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (mEndOfDxe) {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Cannot init context after end of DXE!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // Copy the input communicate buffer payload to the pre-allocated SMM variable payload buffer.
      //
      CopyMem (mVariableBufferPayload, SmmVariableFunctionHeader->Data, CommBufferPayloadSize);
      RuntimeVariableCacheContext = (SMM_VARIABLE_COMMUNICATE_RUNTIME_VARIABLE_CACHE_CONTEXT *)mVariableBufferPayload;

      //
      // Verify required runtime cache buffers are provided.
      //
      if ((RuntimeVariableCacheContext->RuntimeVolatileCache == NULL) ||
          (RuntimeVariableCacheContext->RuntimeNvCache == NULL) ||
          (RuntimeVariableCacheContext->PendingUpdate == NULL) ||
          (RuntimeVariableCacheContext->ReadLock == NULL) ||
          (RuntimeVariableCacheContext->HobFlushComplete == NULL))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Required runtime cache buffer is NULL!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // Verify minimum size requirements for the runtime variable store buffers.
      //
      if (((RuntimeVariableCacheContext->RuntimeHobCache != NULL) &&
           (RuntimeVariableCacheContext->RuntimeHobCache->Size < sizeof (VARIABLE_STORE_HEADER))) ||
          (RuntimeVariableCacheContext->RuntimeVolatileCache->Size < sizeof (VARIABLE_STORE_HEADER)) ||
          (RuntimeVariableCacheContext->RuntimeNvCache->Size < sizeof (VARIABLE_STORE_HEADER)))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: A runtime cache buffer size is invalid!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      //
      // Verify runtime buffers do not overlap with SMRAM ranges.
      //
      if ((RuntimeVariableCacheContext->RuntimeHobCache != NULL) &&
          !VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->RuntimeHobCache,
             (UINTN)RuntimeVariableCacheContext->RuntimeHobCache->Size
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime HOB cache buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (!VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->RuntimeVolatileCache,
             (UINTN)RuntimeVariableCacheContext->RuntimeVolatileCache->Size
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime volatile cache buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (!VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->RuntimeNvCache,
             (UINTN)RuntimeVariableCacheContext->RuntimeNvCache->Size
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime non-volatile cache buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (!VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->PendingUpdate,
             sizeof (*(RuntimeVariableCacheContext->PendingUpdate))
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime cache pending update buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (!VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->ReadLock,
             sizeof (*(RuntimeVariableCacheContext->ReadLock))
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime cache read lock buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      if (!VariableSmmIsNonPrimaryBufferValid (
             (UINTN)RuntimeVariableCacheContext->HobFlushComplete,
             sizeof (*(RuntimeVariableCacheContext->HobFlushComplete))
             ))
      {
        DEBUG ((DEBUG_ERROR, "InitRuntimeVariableCacheContext: Runtime cache HOB flush complete buffer in SMRAM or overflow!\n"));
        Status = EFI_ACCESS_DENIED;
        goto EXIT;
      }

      Status = mVariableStorage->InitCache (
                                   RuntimeVariableCacheContext->RuntimeHobCache,
                                   RuntimeVariableCacheContext->RuntimeVolatileCache,
                                   RuntimeVariableCacheContext->RuntimeNvCache,
                                   RuntimeVariableCacheContext->PendingUpdate,
                                   RuntimeVariableCacheContext->ReadLock,
                                   RuntimeVariableCacheContext->HobFlushComplete
                                   );
      break;

    case SMM_VARIABLE_FUNCTION_SYNC_RUNTIME_CACHE:
      Status = mVariableStorage->SyncCache ();
      break;

    case SMM_VARIABLE_FUNCTION_GET_RUNTIME_CACHE_INFO:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO)) {
        DEBUG ((DEBUG_ERROR, "GetRuntimeCacheInfo: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      GetRuntimeCacheInfo = (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO *)SmmVariableFunctionHeader->Data;

      GetRuntimeCacheInfo->AuthenticatedVariableUsage = mVariableStorage->CheckAuthFormat ();

      Status = mVariableStorage->GetCacheInfo (
                                   &GetRuntimeCacheInfo->TotalHobStorageSize,
                                   &GetRuntimeCacheInfo->TotalVolatileStorageSize,
                                   &GetRuntimeCacheInfo->TotalNvStorageSize
                                   );
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
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  DEBUG ((DEBUG_INFO, "[Variable]SMM_END_OF_DXE is signaled\n"));

  if (!mEndOfDxe) {
    mEndOfDxe = TRUE;
    return mVariableStorage->EndOfDxe ();
  }

  return EFI_SUCCESS;
}

/**
  Variable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols
  for variable read and write services being available. It also registers
  a notification function for an EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
MmVariableServiceInitialize (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  VariableHandle;
  VOID        *SmmEndOfDxeRegistration;

  Status = gMmst->MmLocateProtocol (
                    &gEdkiiVariableStorageProtocolGuid,
                    NULL,
                    (VOID **)&mVariableStorage
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Install the Smm Variable Protocol on a new handle.
  //
  VariableHandle = NULL;
  Status         = gMmst->MmInstallProtocolInterface (
                            &VariableHandle,
                            &gEfiSmmVariableProtocolGuid,
                            EFI_NATIVE_INTERFACE,
                            &gSmmVariable
                            );
  ASSERT_EFI_ERROR (Status);

  mVariableBufferPayloadSize = mVariableStorage->GetMaxVariableSize () +
                               OFFSET_OF (SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY, Name) -
                               GetVariableHeaderSize ();

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    mVariableBufferPayloadSize,
                    (VOID **)&mVariableBufferPayload
                    );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Register SMM variable SMI handler
  ///
  VariableHandle = NULL;
  Status         = gMmst->MmiHandlerRegister (SmmVariableHandler, &gEfiSmmVariableProtocolGuid, &VariableHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // Notify the variable wrapper driver the variable service is ready
  //
  VariableNotifySmmReady ();

  //
  // Register EFI_SMM_END_OF_DXE_PROTOCOL_GUID notify function.
  //
  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiMmEndOfDxeProtocolGuid,
                    SmmEndOfDxeCallback,
                    &SmmEndOfDxeRegistration
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Notify the variable wrapper driver the variable service is ready
  //
  Status = mVariableStorage->InitWriteService (VariableNotifySmmWriteReady);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
