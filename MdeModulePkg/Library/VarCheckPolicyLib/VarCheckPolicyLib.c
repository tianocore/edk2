/** @file -- VarCheckPolicyLib.c
This is an instance of a VarCheck lib that leverages the business logic behind
the VariablePolicy code to make its decisions.

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

//================================================
// As a VarCheck library, we're linked into the VariableServices
// and may not be able to call them indirectly. To get around this,
// use the internal GetVariable function to query the variable store.
//================================================
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  );


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
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
  )
{
  EFI_STATUS                                Status;
  EFI_STATUS                                SubCommandStatus;
  VAR_CHECK_POLICY_COMM_HEADER              *PolicyCommmHeader;
  VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS   *IsEnabledParams;
  VAR_CHECK_POLICY_COMM_DUMP_PARAMS         *DumpParams;
  UINT8                                     *DumpInputBuffer;
  UINT8                                     *DumpOutputBuffer;
  UINTN                                     DumpTotalPages;
  VARIABLE_POLICY_ENTRY                     *PolicyEntry;
  UINTN                                     ExpectedSize;
  // Pagination Cache Variables
  static UINT8                              *PaginationCache = NULL;
  static UINTN                              PaginationCacheSize = 0;
  static UINT32                             CurrentPaginationCommand = 0;

  Status = EFI_SUCCESS;

  //
  // Validate some input parameters.
  //
  // If either of the pointers are NULL, we can't proceed.
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    DEBUG(( DEBUG_INFO, "%a - Invalid comm buffer pointers!\n", __FUNCTION__ ));
    return EFI_INVALID_PARAMETER;
  }
  // If the size does not meet a minimum threshold, we cannot proceed.
  ExpectedSize = sizeof(VAR_CHECK_POLICY_COMM_HEADER);
  if (*CommBufferSize < ExpectedSize) {
    DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
    return EFI_INVALID_PARAMETER;
  }
  // Check the revision and the signature of the comm header.
  PolicyCommmHeader = CommBuffer;
  if (PolicyCommmHeader->Signature != VAR_CHECK_POLICY_COMM_SIG ||
      PolicyCommmHeader->Revision != VAR_CHECK_POLICY_COMM_REVISION) {
    DEBUG(( DEBUG_INFO, "%a - Signature or revision are incorrect!\n", __FUNCTION__ ));
    // We have verified the buffer is not null and have enough size to hold Result field.
    PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
    return EFI_SUCCESS;
  }

  // If we're in the middle of a paginated dump and any other command is sent,
  // pagination cache must be cleared.
  if (PaginationCache != NULL && PolicyCommmHeader->Command != CurrentPaginationCommand) {
    FreePool (PaginationCache);
    PaginationCache = NULL;
    PaginationCacheSize = 0;
    CurrentPaginationCommand = 0;
  }

  //
  // Now we can process the command as it was sent.
  //
  PolicyCommmHeader->Result = EFI_ABORTED;    // Set a default return for incomplete commands.
  switch(PolicyCommmHeader->Command) {
    case VAR_CHECK_POLICY_COMMAND_DISABLE:
      PolicyCommmHeader->Result = DisableVariablePolicy();
      break;

    case VAR_CHECK_POLICY_COMMAND_IS_ENABLED:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof(VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS);
      if (*CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // Now that we know we've got a valid size, we can fill in the rest of the data.
      IsEnabledParams = (VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS*)((UINT8*)CommBuffer + sizeof(VAR_CHECK_POLICY_COMM_HEADER));
      IsEnabledParams->State = IsVariablePolicyEnabled();
      PolicyCommmHeader->Result = EFI_SUCCESS;
      break;

    case VAR_CHECK_POLICY_COMMAND_REGISTER:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof(VARIABLE_POLICY_ENTRY);
      if (*CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // At the very least, we can assume that we're working with a valid policy entry.
      // Time to compare its internal size.
      PolicyEntry = (VARIABLE_POLICY_ENTRY*)((UINT8*)CommBuffer + sizeof(VAR_CHECK_POLICY_COMM_HEADER));
      if (PolicyEntry->Version != VARIABLE_POLICY_ENTRY_REVISION ||
          PolicyEntry->Size < sizeof(VARIABLE_POLICY_ENTRY) ||
          EFI_ERROR(SafeUintnAdd(sizeof(VAR_CHECK_POLICY_COMM_HEADER), PolicyEntry->Size, &ExpectedSize)) ||
          *CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad policy entry contents!\n", __FUNCTION__ ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      PolicyCommmHeader->Result = RegisterVariablePolicy( PolicyEntry );
      break;

    case VAR_CHECK_POLICY_COMMAND_DUMP:
      // Make sure that we're dealing with a reasonable size.
      // This add should be safe because these are fixed sizes so far.
      ExpectedSize += sizeof(VAR_CHECK_POLICY_COMM_DUMP_PARAMS) + VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
      if (*CommBufferSize < ExpectedSize) {
        DEBUG(( DEBUG_INFO, "%a - Bad comm buffer size! %d < %d\n", __FUNCTION__, *CommBufferSize, ExpectedSize ));
        PolicyCommmHeader->Result = EFI_INVALID_PARAMETER;
        break;
      }

      // Now that we know we've got a valid size, we can fill in the rest of the data.
      DumpParams = (VAR_CHECK_POLICY_COMM_DUMP_PARAMS*)(PolicyCommmHeader + 1);

      // If we're requesting the first page, initialize the cache and get the sizes.
      if (DumpParams->PageRequested == 0) {
        if (PaginationCache != NULL) {
          FreePool (PaginationCache);
          PaginationCache = NULL;
        }

        // Determine what the required size is going to be.
        DumpParams->TotalSize = 0;
        DumpParams->PageSize = 0;
        DumpParams->HasMore = FALSE;
        SubCommandStatus = DumpVariablePolicy (NULL, &DumpParams->TotalSize);
        if (SubCommandStatus == EFI_BUFFER_TOO_SMALL && DumpParams->TotalSize > 0) {
          CurrentPaginationCommand = VAR_CHECK_POLICY_COMMAND_DUMP;
          PaginationCacheSize = DumpParams->TotalSize;
          PaginationCache = AllocatePool (PaginationCacheSize);
          if (PaginationCache == NULL) {
            SubCommandStatus = EFI_OUT_OF_RESOURCES;
          }
        }

        // If we've allocated our pagination cache, we're good to cache.
        if (PaginationCache != NULL) {
          SubCommandStatus = DumpVariablePolicy (PaginationCache, &DumpParams->TotalSize);
        }

        // Populate the remaining fields and we can boogie.
        if (!EFI_ERROR (SubCommandStatus) && PaginationCache != NULL) {
          DumpParams->HasMore = TRUE;
        }
      }
      else if (PaginationCache != NULL) {
        DumpParams->TotalSize = (UINT32)PaginationCacheSize;
        DumpParams->PageSize = VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE;
        DumpOutputBuffer = (UINT8*)(DumpParams + 1);

        // Make sure that we don't over-index the cache.
        DumpTotalPages = PaginationCacheSize / DumpParams->PageSize;
        if (PaginationCacheSize % DumpParams->PageSize) DumpTotalPages++;
        if (DumpParams->PageRequested > DumpTotalPages) {
          SubCommandStatus = EFI_INVALID_PARAMETER;
        }
        else {
          // Figure out how far into the page cache we need to go for our next page.
          // We know the blind subtraction won't be bad because we already checked for page 0.
          DumpInputBuffer = &PaginationCache[DumpParams->PageSize * (DumpParams->PageRequested - 1)];
          // If we're getting the last page, adjust the PageSize.
          if (DumpParams->PageRequested == DumpTotalPages) {
            DumpParams->PageSize = PaginationCacheSize % DumpParams->PageSize;
          }
          CopyMem (DumpOutputBuffer, DumpInputBuffer, DumpParams->PageSize);
          // If we just got the last page, settle up the cache.
          if (DumpParams->PageRequested == DumpTotalPages) {
            DumpParams->HasMore = FALSE;
            FreePool (PaginationCache);
            PaginationCache = NULL;
            PaginationCacheSize = 0;
            CurrentPaginationCommand = 0;
          }
          // Otherwise, we could do more here.
          else {
            DumpParams->HasMore = TRUE;
          }

          // If we made it this far, we're basically good.
          SubCommandStatus = EFI_SUCCESS;
        }
      }
      // If we've requested any other page than 0 and the cache is empty, we must have timed out.
      else {
        DumpParams->TotalSize = 0;
        DumpParams->PageSize = 0;
        DumpParams->HasMore = FALSE;
        SubCommandStatus = EFI_TIMEOUT;
      }

      // There's currently no use for this, but it shouldn't be hard to implement.
      PolicyCommmHeader->Result = SubCommandStatus;
      break;

    case VAR_CHECK_POLICY_COMMAND_LOCK:
      PolicyCommmHeader->Result = LockVariablePolicy();
      break;

    default:
      // Mark unknown requested command as EFI_UNSUPPORTED.
      DEBUG(( DEBUG_INFO, "%a - Invalid command requested! %d\n", __FUNCTION__, PolicyCommmHeader->Command ));
      PolicyCommmHeader->Result = EFI_UNSUPPORTED;
      break;
  }

  DEBUG(( DEBUG_VERBOSE, "%a - Command %d returning %r.\n", __FUNCTION__,
          PolicyCommmHeader->Command, PolicyCommmHeader->Result ));

  return Status;
}


/**
  Constructor function of VarCheckPolicyLib to register VarCheck handler and
  SW MMI handlers.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPolicyLibConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    DiscardedHandle;

  // Initialize the business logic with the internal GetVariable handler.
  Status = InitVariablePolicyLib( VariableServiceGetVariable );

  // Only proceed with init if the business logic could be initialized.
  if (!EFI_ERROR( Status )) {
    // Register the VarCheck handler for SetVariable filtering.
    // Forward the check to the business logic of the library.
    VarCheckLibRegisterSetVariableCheckHandler( ValidateSetVariable );

    // Register the MMI handlers for receiving policy commands.
    DiscardedHandle = NULL;
    Status = gMmst->MmiHandlerRegister( VarCheckPolicyLibMmiHandler,
                                        &gVarCheckPolicyLibMmiHandlerGuid,
                                        &DiscardedHandle );
  }
  // Otherwise, there's not much we can do.
  else {
    DEBUG(( DEBUG_ERROR, "%a - Cannot Initialize VariablePolicyLib! %r\n", __FUNCTION__, Status ));
    ASSERT_EFI_ERROR( Status );
  }

  return Status;
}
