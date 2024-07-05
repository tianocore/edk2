/** @file -- VarCheckPolicyLib.c
This is a NULL library instance that leverages the VarCheck interface
and the business logic behind the VariablePolicy code to make its decisions.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/MmCommunication.h>

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>

#include <Guid/VarCheckPolicyMmi.h>

#include "VarCheckPolicyLib.h"

// ================================================
// As a VarCheck library, we're linked into the VariableServices
// and may not be able to call them indirectly. To get around this,
// use the internal GetVariable function to query the variable store.
// ================================================
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data
  );

UINT8  mSecurityEvalBuffer[VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE];

// Pagination Cache Variables
UINT8   *mPaginationCache         = NULL;
UINTN   mPaginationCacheSize      = 0;
UINT32  mCurrentPaginationCommand = 0;

/**
  MM Communication Handler to recieve commands from the DXE protocol for
  Variable Policies. This communication channel is used to register new policies
  and poll and toggle the enforcement of variable policies.

  @param[in]      DispatchHandle      All parameters standard to MM communications convention.
  @param[in]      RegisterContext     All parameters standard to MM communications convention.
  @param[in,out]  CommBuffer          All parameters standard to MM communications convention.
  @param[in,out]  CommBufferSize      All parameters standard to MM communications convention.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   CommBuffer or CommBufferSize is null pointer.
  @retval     EFI_INVALID_PARAMETER   CommBuffer size is wrong.
  @retval     EFI_INVALID_PARAMETER   Revision or signature don't match.

**/
STATIC
EFI_STATUS
EFIAPI
VarCheckPolicyLibMmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *RegisterContext,
  IN OUT VOID        *CommBuffer,
  IN OUT UINTN       *CommBufferSize
  )
{
  UINTN                                    InternalCommBufferSize;
  VOID                                     *InternalCommBuffer;
  EFI_STATUS                               Status;
  EFI_STATUS                               SubCommandStatus;
  VAR_CHECK_POLICY_COMM_HEADER             *PolicyCommHeader;
  VAR_CHECK_POLICY_COMM_HEADER             *InternalPolicyCommHeader;
  VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS  *IsEnabledParams;
  VAR_CHECK_POLICY_COMM_DUMP_PARAMS        *DumpParamsIn;
  VAR_CHECK_POLICY_COMM_DUMP_PARAMS        *DumpParamsOut;
  VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS    *GetInfoParamsInternal;
  VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS    *GetInfoParamsExternal;
  CHAR16                                   *InternalCopyOfOutputVariableName;
  CHAR16                                   *ExternalCopyOfOutputVariableName;
  UINT8                                    *DumpInputBuffer;
  UINT8                                    *DumpOutputBuffer;
  UINTN                                    AllowedOutputVariableNameSize;
  UINTN                                    DumpTotalPages;
  UINTN                                    LocalSize;
  VARIABLE_POLICY_ENTRY                    *PolicyEntry;
  UINTN                                    ExpectedSize;
  UINT32                                   TempSize;

  Status = EFI_SUCCESS;

  //
  // Validate some input parameters.
  //
  // If either of the pointers are NULL, we can't proceed.
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    DEBUG ((DEBUG_INFO, "%a - Invalid comm buffer pointers!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure that the buffer is valid.
  // This should be covered by the SmiManage infrastructure, but just to be safe...
  //
  InternalCommBufferSize = *CommBufferSize;
  if ((InternalCommBufferSize > VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE) ||
      !VarCheckPolicyIsPrimaryBufferValid ((UINTN)CommBuffer, (UINT64)InternalCommBufferSize))
  {
    DEBUG ((DEBUG_ERROR, "%a - Invalid Primary Buffer (CommBuffer) supplied! 0x%016lX[0x%016lX]\n", __func__, CommBuffer, InternalCommBufferSize));
    return EFI_INVALID_PARAMETER;
  }

  // If the size does not meet a minimum threshold, we cannot proceed.
  ExpectedSize = sizeof (VAR_CHECK_POLICY_COMM_HEADER);
  if (InternalCommBufferSize < ExpectedSize) {
    DEBUG ((DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __func__, InternalCommBufferSize, ExpectedSize));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Before proceeding any further, copy the buffer internally so that we can compare
  // without worrying about TOCTOU.
  //
  InternalCommBuffer = &mSecurityEvalBuffer[0];
  CopyMem (InternalCommBuffer, CommBuffer, InternalCommBufferSize);
  PolicyCommHeader         = CommBuffer;
  InternalPolicyCommHeader = InternalCommBuffer;
  // Check the revision and the signature of the comm header.
  if ((InternalPolicyCommHeader->Signature != VAR_CHECK_POLICY_COMM_SIG) ||
      (InternalPolicyCommHeader->Revision != VAR_CHECK_POLICY_COMM_REVISION))
  {
    DEBUG ((DEBUG_INFO, "%a - Signature or revision are incorrect!\n", __func__));
    // We have verified the buffer is not null and have enough size to hold Result field.
    PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  // If we're in the middle of a paginated dump and any other command is sent,
  // pagination cache must be cleared.
  if ((mPaginationCache != NULL) && (InternalPolicyCommHeader->Command != mCurrentPaginationCommand)) {
    FreePool (mPaginationCache);
    mPaginationCache          = NULL;
    mPaginationCacheSize      = 0;
    mCurrentPaginationCommand = 0;
  }

  //
  // Now we can process the command as it was sent.
  //
  PolicyCommHeader->Result = EFI_ABORTED;    // Set a default return for incomplete commands.
  switch (InternalPolicyCommHeader->Command) {
    case VAR_CHECK_POLICY_COMMAND_DISABLE:
      PolicyCommHeader->Result = DisableVariablePolicy ();
      break;

    case VAR_CHECK_POLICY_COMMAND_IS_ENABLED:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS);
      if (InternalCommBufferSize < ExpectedSize) {
        DEBUG ((DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __func__, InternalCommBufferSize, ExpectedSize));
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // Now that we know we've got a valid size, we can fill in the rest of the data.
      IsEnabledParams          = (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS *)((UINT8 *)CommBuffer + sizeof (VAR_CHECK_POLICY_COMM_HEADER));
      IsEnabledParams->State   = IsVariablePolicyEnabled ();
      PolicyCommHeader->Result = EFI_SUCCESS;
      break;

    case VAR_CHECK_POLICY_COMMAND_REGISTER:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof (VARIABLE_POLICY_ENTRY);
      if (InternalCommBufferSize < ExpectedSize) {
        DEBUG ((DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __func__, InternalCommBufferSize, ExpectedSize));
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // At the very least, we can assume that we're working with a valid policy entry.
      // Time to compare its internal size.
      PolicyEntry = (VARIABLE_POLICY_ENTRY *)((UINT8 *)InternalCommBuffer + sizeof (VAR_CHECK_POLICY_COMM_HEADER));
      if ((PolicyEntry->Version != VARIABLE_POLICY_ENTRY_REVISION) ||
          (PolicyEntry->Size < sizeof (VARIABLE_POLICY_ENTRY)) ||
          EFI_ERROR (SafeUintnAdd (sizeof (VAR_CHECK_POLICY_COMM_HEADER), PolicyEntry->Size, &ExpectedSize)) ||
          (InternalCommBufferSize < ExpectedSize))
      {
        DEBUG ((DEBUG_INFO, "%a - Bad policy entry contents!\n", __func__));
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      PolicyCommHeader->Result = RegisterVariablePolicy (PolicyEntry);
      break;

    case VAR_CHECK_POLICY_COMMAND_DUMP:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof (VAR_CHECK_POLICY_COMM_DUMP_PARAMS) + VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
      if (InternalCommBufferSize < ExpectedSize) {
        DEBUG ((DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __func__, InternalCommBufferSize, ExpectedSize));
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // Now that we know we've got a valid size, we can fill in the rest of the data.
      DumpParamsIn  = (VAR_CHECK_POLICY_COMM_DUMP_PARAMS *)(InternalPolicyCommHeader + 1);
      DumpParamsOut = (VAR_CHECK_POLICY_COMM_DUMP_PARAMS *)(PolicyCommHeader + 1);

      // If we're requesting the first page, initialize the cache and get the sizes.
      if (DumpParamsIn->PageRequested == 0) {
        if (mPaginationCache != NULL) {
          FreePool (mPaginationCache);
          mPaginationCache = NULL;
        }

        // Determine what the required size is going to be.
        DumpParamsOut->TotalSize = 0;
        DumpParamsOut->PageSize  = 0;
        DumpParamsOut->HasMore   = FALSE;
        TempSize                 = 0;
        SubCommandStatus         = DumpVariablePolicy (NULL, &TempSize);
        if ((SubCommandStatus == EFI_BUFFER_TOO_SMALL) && (TempSize > 0)) {
          mCurrentPaginationCommand = VAR_CHECK_POLICY_COMMAND_DUMP;
          mPaginationCacheSize      = TempSize;
          DumpParamsOut->TotalSize  = TempSize;
          mPaginationCache          = AllocatePool (mPaginationCacheSize);
          if (mPaginationCache == NULL) {
            SubCommandStatus = EFI_OUT_OF_RESOURCES;
          }
        }

        // If we've allocated our pagination cache, we're good to cache.
        if (mPaginationCache != NULL) {
          SubCommandStatus = DumpVariablePolicy (mPaginationCache, &TempSize);
        }

        // Populate the remaining fields and we can boogie.
        if (!EFI_ERROR (SubCommandStatus) && (mPaginationCache != NULL)) {
          DumpParamsOut->HasMore = TRUE;
        }
      } else if (mPaginationCache != NULL) {
        DumpParamsOut->TotalSize = (UINT32)mPaginationCacheSize;
        DumpOutputBuffer         = (UINT8 *)(DumpParamsOut + 1);

        // Make sure that we don't over-index the cache.
        DumpTotalPages = mPaginationCacheSize / VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
        if (mPaginationCacheSize % VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE != 0) {
          DumpTotalPages++;
        }

        if (DumpParamsIn->PageRequested > DumpTotalPages) {
          SubCommandStatus = EFI_INVALID_PARAMETER;
        } else {
          // Figure out how far into the page cache we need to go for our next page.
          // We know the blind subtraction won't be bad because we already checked for page 0.
          DumpInputBuffer = &mPaginationCache[VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE * (DumpParamsIn->PageRequested - 1)];
          TempSize        = VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
          // If we're getting the last page, adjust the PageSize.
          if (DumpParamsIn->PageRequested == DumpTotalPages) {
            TempSize = mPaginationCacheSize % VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
          }

          CopyMem (DumpOutputBuffer, DumpInputBuffer, TempSize);
          DumpParamsOut->PageSize = TempSize;
          // If we just got the last page, settle up the cache.
          if (DumpParamsIn->PageRequested == DumpTotalPages) {
            DumpParamsOut->HasMore = FALSE;
            FreePool (mPaginationCache);
            mPaginationCache          = NULL;
            mPaginationCacheSize      = 0;
            mCurrentPaginationCommand = 0;
            // Otherwise, we could do more here.
          } else {
            DumpParamsOut->HasMore = TRUE;
          }

          // If we made it this far, we're basically good.
          SubCommandStatus = EFI_SUCCESS;
        }

        // If we've requested any other page than 0 and the cache is empty, we must have timed out.
      } else {
        DumpParamsOut->TotalSize = 0;
        DumpParamsOut->PageSize  = 0;
        DumpParamsOut->HasMore   = FALSE;
        SubCommandStatus         = EFI_TIMEOUT;
      }

      // There's currently no use for this, but it shouldn't be hard to implement.
      PolicyCommHeader->Result = SubCommandStatus;
      break;

    case VAR_CHECK_POLICY_COMMAND_LOCK:
      PolicyCommHeader->Result = LockVariablePolicy ();
      break;

    case VAR_CHECK_POLICY_COMMAND_GET_INFO:
    case VAR_CHECK_POLICY_COMMAND_GET_LOCK_VAR_STATE_INFO:
      ExpectedSize += VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS_END + VAR_CHECK_POLICY_MM_GET_INFO_BUFFER_SIZE;

      if (InternalCommBufferSize < ExpectedSize) {
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      GetInfoParamsInternal = (VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS *)(InternalPolicyCommHeader + 1);
      GetInfoParamsExternal = (VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS *)(PolicyCommHeader + 1);

      SubCommandStatus =  SafeUintnSub (
                            VAR_CHECK_POLICY_MM_GET_INFO_BUFFER_SIZE,
                            GetInfoParamsInternal->InputVariableNameSize,
                            &AllowedOutputVariableNameSize
                            );
      if (EFI_ERROR (SubCommandStatus)) {
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      if (GetInfoParamsInternal->OutputVariableNameSize > 0) {
        SubCommandStatus =  SafeUintnAdd (
                              ((UINTN)GetInfoParamsInternal + VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS_END),
                              (UINTN)GetInfoParamsInternal->InputVariableNameSize,
                              (UINTN *)&InternalCopyOfOutputVariableName
                              );
        if (EFI_ERROR (SubCommandStatus)) {
          PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
          break;
        }
      } else {
        InternalCopyOfOutputVariableName = NULL;
      }

      ZeroMem (&GetInfoParamsInternal->OutputPolicyEntry, sizeof (GetInfoParamsInternal->OutputPolicyEntry));
      ZeroMem (&GetInfoParamsExternal->OutputPolicyEntry, sizeof (GetInfoParamsExternal->OutputPolicyEntry));

      LocalSize = (UINTN)GetInfoParamsInternal->OutputVariableNameSize;

      if (InternalPolicyCommHeader->Command == VAR_CHECK_POLICY_COMMAND_GET_INFO) {
        SubCommandStatus =  GetVariablePolicyInfo (
                              GetInfoParamsInternal->InputVariableName,
                              &GetInfoParamsInternal->InputVendorGuid,
                              &LocalSize,
                              &GetInfoParamsInternal->OutputPolicyEntry.VariablePolicy,
                              InternalCopyOfOutputVariableName
                              );
      } else if (InternalPolicyCommHeader->Command == VAR_CHECK_POLICY_COMMAND_GET_LOCK_VAR_STATE_INFO) {
        SubCommandStatus =  GetLockOnVariableStateVariablePolicyInfo (
                              GetInfoParamsInternal->InputVariableName,
                              &GetInfoParamsInternal->InputVendorGuid,
                              &LocalSize,
                              &GetInfoParamsInternal->OutputPolicyEntry.LockOnVarStatePolicy,
                              InternalCopyOfOutputVariableName
                              );
      } else {
        PolicyCommHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      if (EFI_ERROR (SubCommandStatus) && (SubCommandStatus != EFI_BUFFER_TOO_SMALL)) {
        PolicyCommHeader->Result = SubCommandStatus;
        break;
      }

      if (EFI_ERROR (SafeUintnToUint32 (LocalSize, &GetInfoParamsInternal->OutputVariableNameSize))) {
        PolicyCommHeader->Result = EFI_BAD_BUFFER_SIZE;
        break;
      }

      ASSERT (sizeof (GetInfoParamsInternal->OutputPolicyEntry) == sizeof (GetInfoParamsExternal->OutputPolicyEntry));
      CopyMem (
        &GetInfoParamsExternal->OutputPolicyEntry,
        &GetInfoParamsInternal->OutputPolicyEntry,
        sizeof (GetInfoParamsExternal->OutputPolicyEntry)
        );

      GetInfoParamsExternal->OutputVariableNameSize = GetInfoParamsInternal->OutputVariableNameSize;
      if (SubCommandStatus == EFI_BUFFER_TOO_SMALL) {
        PolicyCommHeader->Result = EFI_BUFFER_TOO_SMALL;
        break;
      }

      SubCommandStatus =  SafeUintnAdd (
                            ((UINTN)GetInfoParamsExternal + VAR_CHECK_POLICY_COMM_GET_INFO_PARAMS_END),
                            (UINTN)GetInfoParamsInternal->InputVariableNameSize,
                            (UINTN *)&ExternalCopyOfOutputVariableName
                            );
      if (EFI_ERROR (SubCommandStatus)) {
        PolicyCommHeader->Result = EFI_BAD_BUFFER_SIZE;
        break;
      }

      if (GetInfoParamsInternal->OutputVariableNameSize > 0) {
        SubCommandStatus =  StrnCpyS (
                              ExternalCopyOfOutputVariableName,
                              AllowedOutputVariableNameSize,
                              InternalCopyOfOutputVariableName,
                              (UINTN)GetInfoParamsInternal->OutputVariableNameSize
                              );
        ASSERT_EFI_ERROR (SubCommandStatus);
      } else {
        // The comm buffer should always have the space for the variable policy output
        // variable name. Fill it with NULL chars if a variable name is not present so
        // it has a consistent value in the case of variable name absence.
        SetMem (ExternalCopyOfOutputVariableName, AllowedOutputVariableNameSize, CHAR_NULL);
      }

      PolicyCommHeader->Result = SubCommandStatus;

      break;

    default:
      // Mark unknown requested command as EFI_UNSUPPORTED.
      DEBUG ((DEBUG_INFO, "%a - Invalid command requested! %d\n", __func__, PolicyCommHeader->Command));
      PolicyCommHeader->Result = EFI_UNSUPPORTED;
      break;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a - Command %d returning %r.\n",
    __func__,
    PolicyCommHeader->Command,
    PolicyCommHeader->Result
    ));

  return Status;
}

/**
  Constructor function of VarCheckPolicyLib to register VarCheck handler and
  SW MMI handlers.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibCommonConstructor (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DiscardedHandle;

  // Initialize the business logic with the internal GetVariable handler.
  Status = InitVariablePolicyLib (VariableServiceGetVariable);

  // Only proceed with init if the business logic could be initialized.
  if (!EFI_ERROR (Status)) {
    // Register the VarCheck handler for SetVariable filtering.
    // Forward the check to the business logic of the library.
    VarCheckLibRegisterSetVariableCheckHandler (ValidateSetVariable);

    // Register the MMI handlers for receiving policy commands.
    DiscardedHandle = NULL;
    Status          = gMmst->MmiHandlerRegister (
                               VarCheckPolicyLibMmiHandler,
                               &gVarCheckPolicyLibMmiHandlerGuid,
                               &DiscardedHandle
                               );
  }
  // Otherwise, there's not much we can do.
  else {
    DEBUG ((DEBUG_ERROR, "%a - Cannot Initialize VariablePolicyLib! %r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
