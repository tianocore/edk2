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

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/VariableStorageRouterLib.h>

#include <Guid/SmmVariableCommon.h>
#include "PrivilegePolymorphic.h"

extern VARIABLE_STORE_HEADER  *mNvVariableCache;

STATIC BOOLEAN  mAtRuntime              = FALSE;
STATIC BOOLEAN  mEndOfDxe               = FALSE;
STATIC UINT8    *mVariableBufferPayload = NULL;
STATIC UINTN    mVariableBufferPayloadSize;

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

  AuthFormat = VariableStorageRouterLibCheckAuthFormat ();

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
SmmVariableSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  return VariableStorageRouterLibSetVariable (
           VariableName,
           VendorGuid,
           Attributes,
           DataSize,
           Data,
           TRUE
           );
}

EFI_SMM_VARIABLE_PROTOCOL  gSmmVariable = {
  VariableStorageRouterLibGetVariable,
  VariableStorageRouterLibGetNextVariableName,
  SmmVariableSetVariable,
  VariableStorageRouterLibQueryVariableInfo
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

      Status = VariableStorageRouterLibGetVariable (
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

      Status = VariableStorageRouterLibGetNextVariableName (
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

      Status = VariableStorageRouterLibSetVariable (
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

      Status = VariableStorageRouterLibQueryVariableInfo (
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
        Status = VariableStorageRouterLibReadyToBoot ();
      } else {
        Status = EFI_SUCCESS;
      }

      break;

    case SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE:
      mAtRuntime = TRUE;
      Status     = VariableStorageRouterLibExitBootService ();
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

      Status          = VariableStorageRouterLibGetStatics (VariableInfo, &InfoSize);
      *CommBufferSize = InfoSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
      break;

    case SMM_VARIABLE_FUNCTION_LOCK_VARIABLE:
      if (mEndOfDxe) {
        Status = EFI_ACCESS_DENIED;
      } else {
        VariableToLock = (SMM_VARIABLE_COMMUNICATE_LOCK_VARIABLE *)SmmVariableFunctionHeader->Data;
        Status         = VariableStorageRouterLibRequestToLock (
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
        Status               = VariableStorageRouterLibPropertySet (
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

      Status = VariableStorageRouterLibPropertyGet (
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

      Status = VariableStorageRouterLibInitCache (
                 RuntimeVariableCacheContext->RuntimeHobCache,
                 RuntimeVariableCacheContext->RuntimeVolatileCache,
                 RuntimeVariableCacheContext->RuntimeNvCache,
                 RuntimeVariableCacheContext->PendingUpdate,
                 RuntimeVariableCacheContext->ReadLock,
                 RuntimeVariableCacheContext->HobFlushComplete
                 );
      break;

    case SMM_VARIABLE_FUNCTION_SYNC_RUNTIME_CACHE:
      Status = VariableStorageRouterLibSyncCache ();
      break;

    case SMM_VARIABLE_FUNCTION_GET_RUNTIME_CACHE_INFO:
      if (CommBufferPayloadSize < sizeof (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO)) {
        DEBUG ((DEBUG_ERROR, "GetRuntimeCacheInfo: SMM communication buffer size invalid!\n"));
        return EFI_SUCCESS;
      }

      GetRuntimeCacheInfo = (SMM_VARIABLE_COMMUNICATE_GET_RUNTIME_CACHE_INFO *)SmmVariableFunctionHeader->Data;

      GetRuntimeCacheInfo->AuthenticatedVariableUsage = VariableStorageRouterLibCheckAuthFormat ();

      Status = VariableStorageRouterLibGetCacheInfo (
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
    return VariableStorageRouterLibEndOfDxe ();
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

  mVariableBufferPayloadSize = VariableStorageRouterLibGetMaxVariableSize () +
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
  Status = VariableStorageRouterLibInitWriteService (VariableNotifySmmWriteReady);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
