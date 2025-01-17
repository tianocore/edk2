/** @file -- VariablePolicyLib.c
Business logic for Variable Policy enforcement.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/SafeIntLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>

// IMPORTANT NOTE: This library is currently rife with multiple return statements
//                 for error handling. A refactor should remove these at some point.

//
// This library was designed with advanced unit-test features.
// This define handles the configuration.
#ifdef INTERNAL_UNIT_TEST
  #undef STATIC
#define STATIC    // Nothing...
#endif

// An abstracted GetVariable interface that enables configuration regardless of the environment.
EFI_GET_VARIABLE  mGetVariableHelper = NULL;

// Master switch to lock this entire interface. Does not stop enforcement,
// just prevents the configuration from being changed for the rest of the boot.
STATIC  BOOLEAN  mInterfaceLocked = FALSE;

// Master switch to disable the entire interface for a single boot.
// This will disable all policy enforcement for the duration of the boot.
STATIC  BOOLEAN  mProtectionDisabled = FALSE;

// Table to hold all the current policies.
UINT8           *mPolicyTable      = NULL;
STATIC  UINT32  mCurrentTableSize  = 0;
STATIC  UINT32  mCurrentTableUsage = 0;
STATIC  UINT32  mCurrentTableCount = 0;

#define POLICY_TABLE_STEP_SIZE  0x1000

// NOTE: DO NOT USE THESE MACROS on any structure that has not been validated.
//       Current table data has already been sanitized.
#define GET_NEXT_POLICY(CurPolicy)  (VARIABLE_POLICY_ENTRY*)((UINT8*)CurPolicy + CurPolicy->Size)
#define GET_POLICY_NAME(CurPolicy)  (CHAR16*)((UINTN)CurPolicy + CurPolicy->OffsetToName)

#define MATCH_PRIORITY_EXACT  0
#define MATCH_PRIORITY_MAX    MATCH_PRIORITY_EXACT
#define MATCH_PRIORITY_MIN    MAX_UINT8

/**
  An extra init hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with init.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraInit (
  VOID
  );

/**
  An extra deinit hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with deinit.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraDeinit (
  VOID
  );

/**
  This helper function determines whether the structure of an incoming policy
  is valid and internally consistent.

  @param[in]  NewPolicy     Pointer to the incoming policy structure.

  @retval     TRUE
  @retval     FALSE   Pointer is NULL, size is wrong, strings are empty, or
                      substructures overlap.

**/
STATIC
BOOLEAN
IsValidVariablePolicyStructure (
  IN CONST VARIABLE_POLICY_ENTRY  *NewPolicy
  )
{
  EFI_STATUS  Status;
  UINTN       EntryEnd;
  CHAR16      *CheckChar;
  UINTN       WildcardCount;

  // Sanitize some quick values.
  if ((NewPolicy == NULL) || (NewPolicy->Size == 0) ||
      // Structure size should be at least as long as the minumum structure and a NULL string.
      (NewPolicy->Size < sizeof (VARIABLE_POLICY_ENTRY)) ||
      // Check for the known revision.
      (NewPolicy->Version != VARIABLE_POLICY_ENTRY_REVISION))
  {
    return FALSE;
  }

  // Calculate the theoretical end of the structure and make sure
  // that the structure can fit in memory.
  Status = SafeUintnAdd ((UINTN)NewPolicy, NewPolicy->Size, &EntryEnd);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Check for a valid Max Size.
  if (NewPolicy->MaxSize == 0) {
    return FALSE;
  }

  // Check for the valid list of lock policies.
  if ((NewPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_NO_LOCK) &&
      (NewPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_NOW) &&
      (NewPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_ON_CREATE) &&
      (NewPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE))
  {
    return FALSE;
  }

  // If the policy type is VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE, make sure that the matching state variable Name
  // terminates before the OffsetToName for the matching policy variable Name.
  if (NewPolicy->LockPolicyType == VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE) {
    // Adjust CheckChar to the offset of the LockPolicy->Name.
    Status = SafeUintnAdd (
               (UINTN)NewPolicy + sizeof (VARIABLE_POLICY_ENTRY),
               sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY),
               (UINTN *)&CheckChar
               );
    if (EFI_ERROR (Status) || (EntryEnd <= (UINTN)CheckChar)) {
      return FALSE;
    }

    while (*CheckChar != CHAR_NULL) {
      if (EntryEnd <= (UINTN)CheckChar) {
        return FALSE;
      }

      CheckChar++;
    }

    // At this point we should have either exeeded the structure or be pointing at the last char in LockPolicy->Name.
    // We should check to make sure that the policy Name comes immediately after this charcter.
    if ((UINTN)++ CheckChar != (UINTN)NewPolicy + NewPolicy->OffsetToName) {
      return FALSE;
    }

    // If the policy type is any other value, make sure that the LockPolicy structure has a zero length.
  } else {
    if (NewPolicy->OffsetToName != sizeof (VARIABLE_POLICY_ENTRY)) {
      return FALSE;
    }
  }

  // Check to make sure that the name has a terminating character
  // before the end of the structure.
  // We've already checked that the name is within the bounds of the structure.
  if (NewPolicy->Size != NewPolicy->OffsetToName) {
    CheckChar     = (CHAR16 *)((UINTN)NewPolicy + NewPolicy->OffsetToName);
    WildcardCount = 0;
    while (*CheckChar != CHAR_NULL) {
      // Make sure there aren't excessive wildcards.
      if (*CheckChar == L'#') {
        WildcardCount++;
        if (WildcardCount > MATCH_PRIORITY_MIN) {
          return FALSE;
        }
      }

      // Make sure you're still within the bounds of the policy structure.
      if (EntryEnd <= (UINTN)CheckChar) {
        return FALSE;
      }

      CheckChar++;
    }

    // Finally, we should be pointed at the very last character in Name, so we should be right
    // up against the end of the structure.
    if ((UINTN)++ CheckChar != EntryEnd) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  This helper function evaluates a policy and determines whether it matches the target
  variable. If matched, will also return a value corresponding to the priority of the match.

  The rules for "best match" are listed in the Variable Policy Spec.
  Perfect name matches will return 0.
  Single wildcard characters will return the number of wildcard characters.
  Full namespaces will return MAX_UINT8.

  @param[in]  EvalEntry         Pointer to the policy entry being evaluated.
  @param[in]  VariableName      Same as EFI_SET_VARIABLE.
  @param[in]  VendorGuid        Same as EFI_SET_VARIABLE.
  @param[out] MatchPriority     [Optional] On finding a match, this value contains the priority of the match.
                                Lower number == higher priority. Only valid if a match found.

  @retval     TRUE          Current entry matches the target variable.
  @retval     FALSE         Current entry does not match at all.

**/
STATIC
BOOLEAN
EvaluatePolicyMatch (
  IN CONST  VARIABLE_POLICY_ENTRY  *EvalEntry,
  IN CONST  CHAR16                 *VariableName,
  IN CONST  EFI_GUID               *VendorGuid,
  OUT       UINT8                  *MatchPriority    OPTIONAL
  )
{
  BOOLEAN  Result;
  CHAR16   *PolicyName;
  UINT8    CalculatedPriority;
  UINTN    Index;

  Result             = FALSE;
  CalculatedPriority = MATCH_PRIORITY_EXACT;

  // Step 1: If the GUID doesn't match, we're done. No need to evaluate anything else.
  if (!CompareGuid (&EvalEntry->Namespace, VendorGuid)) {
    goto Exit;
  }

  // If the GUID matches, check to see whether there is a Name associated
  // with the policy. If not, this policy matches the entire namespace.
  // Missing Name is indicated by size being equal to name.
  if (EvalEntry->Size == EvalEntry->OffsetToName) {
    CalculatedPriority = MATCH_PRIORITY_MIN;
    Result             = TRUE;
    goto Exit;
  }

  // Now that we know the name exists, get it.
  PolicyName = GET_POLICY_NAME (EvalEntry);

  // Evaluate the name against the policy name and check for a match.
  // Account for any wildcards.
  Index  = 0;
  Result = TRUE;
  // Keep going until the end of both strings.
  while (PolicyName[Index] != CHAR_NULL || VariableName[Index] != CHAR_NULL) {
    // If we don't have a match...
    if ((PolicyName[Index] != VariableName[Index]) || (PolicyName[Index] == L'#')) {
      // If this is a numerical wildcard, we can consider
      // it a match if we alter the priority.
      if ((PolicyName[Index] == L'#') &&
          (((L'0' <= VariableName[Index]) && (VariableName[Index] <= L'9')) ||
           ((L'A' <= VariableName[Index]) && (VariableName[Index] <= L'F')) ||
           ((L'a' <= VariableName[Index]) && (VariableName[Index] <= L'f'))))
      {
        if (CalculatedPriority < MATCH_PRIORITY_MIN) {
          CalculatedPriority++;
        }

        // Otherwise, not a match.
      } else {
        Result = FALSE;
        goto Exit;
      }
    }

    Index++;
  }

Exit:
  if (Result && (MatchPriority != NULL)) {
    *MatchPriority = CalculatedPriority;
  }

  return Result;
}

/**
  This helper function walks the current policy table and returns a pointer
  to the best match, if any are found. Leverages EvaluatePolicyMatch() to
  determine "best".

  @param[in]  VariableName       Same as EFI_SET_VARIABLE.
  @param[in]  VendorGuid         Same as EFI_SET_VARIABLE.
  @param[out] ReturnPriority     [Optional] If pointer is provided, return the
                                 priority of the match. Same as EvaluatePolicyMatch().
                                 Only valid if a match is returned.

  @retval     VARIABLE_POLICY_ENTRY*    Best match that was found.
  @retval     NULL                      No match was found.

**/
STATIC
VARIABLE_POLICY_ENTRY *
GetBestPolicyMatch (
  IN CONST  CHAR16    *VariableName,
  IN CONST  EFI_GUID  *VendorGuid,
  OUT       UINT8     *ReturnPriority  OPTIONAL
  )
{
  VARIABLE_POLICY_ENTRY  *BestResult;
  VARIABLE_POLICY_ENTRY  *CurrentEntry;
  UINT8                  MatchPriority;
  UINT8                  CurrentPriority;
  UINTN                  Index;

  BestResult    = NULL;
  MatchPriority = MATCH_PRIORITY_EXACT;

  // Walk all entries in the table, looking for matches.
  CurrentEntry = (VARIABLE_POLICY_ENTRY *)mPolicyTable;
  for (Index = 0; Index < mCurrentTableCount; Index++) {
    // Check for a match.
    if (EvaluatePolicyMatch (CurrentEntry, VariableName, VendorGuid, &CurrentPriority)) {
      // If match is better, take it.
      if ((BestResult == NULL) || (CurrentPriority < MatchPriority)) {
        BestResult    = CurrentEntry;
        MatchPriority = CurrentPriority;
      }

      // If you've hit the highest-priority match, can exit now.
      if (MatchPriority == 0) {
        break;
      }
    }

    // If we're still in the loop, move to the next entry.
    CurrentEntry = GET_NEXT_POLICY (CurrentEntry);
  }

  // If a return priority was requested, return it.
  if (ReturnPriority != NULL) {
    *ReturnPriority = MatchPriority;
  }

  return BestResult;
}

/**
  This API function validates and registers a new policy with
  the policy enforcement engine.

  @param[in]  NewPolicy     Pointer to the incoming policy structure.

  @retval     EFI_SUCCESS
  @retval     EFI_INVALID_PARAMETER   NewPolicy is NULL or is internally inconsistent.
  @retval     EFI_ALREADY_STARTED     An identical matching policy already exists.
  @retval     EFI_WRITE_PROTECTED     The interface has been locked until the next reboot.
  @retval     EFI_UNSUPPORTED         Policy enforcement has been disabled. No reason to add more policies.
  @retval     EFI_ABORTED             A calculation error has prevented this function from completing.
  @retval     EFI_OUT_OF_RESOURCES    Cannot grow the table to hold any more policies.
  @retval     EFI_NOT_READY           Library has not yet been initialized.

**/
EFI_STATUS
EFIAPI
RegisterVariablePolicy (
  IN CONST VARIABLE_POLICY_ENTRY  *NewPolicy
  )
{
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *MatchPolicy;
  UINT8                  MatchPriority;
  UINT32                 NewSize;
  UINT8                  *NewTable;

  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  if (mInterfaceLocked) {
    return EFI_WRITE_PROTECTED;
  }

  if (!IsValidVariablePolicyStructure (NewPolicy)) {
    return EFI_INVALID_PARAMETER;
  }

  // Check to see whether an exact matching policy already exists.
  MatchPolicy = GetBestPolicyMatch (
                  GET_POLICY_NAME (NewPolicy),
                  &NewPolicy->Namespace,
                  &MatchPriority
                  );
  if ((MatchPolicy != NULL) && (MatchPriority == MATCH_PRIORITY_EXACT)) {
    return EFI_ALREADY_STARTED;
  }

  // If none exists, create it.
  // If we need more space, allocate that now.
  Status = SafeUint32Add (mCurrentTableUsage, NewPolicy->Size, &NewSize);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  if (NewSize > mCurrentTableSize) {
    // Use NewSize to calculate the new table size in units of POLICY_TABLE_STEP_SIZE.
    NewSize = (NewSize % POLICY_TABLE_STEP_SIZE) > 0 ?
              (NewSize / POLICY_TABLE_STEP_SIZE) + 1 :
              (NewSize / POLICY_TABLE_STEP_SIZE);
    // Calculate the new table size in absolute bytes.
    Status = SafeUint32Mult (NewSize, POLICY_TABLE_STEP_SIZE, &NewSize);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }

    // Reallocate and copy the table.
    NewTable = AllocateRuntimePool (NewSize);
    if (NewTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewTable, mPolicyTable, mCurrentTableUsage);
    mCurrentTableSize = NewSize;
    if (mPolicyTable != NULL) {
      FreePool (mPolicyTable);
    }

    mPolicyTable = NewTable;
  }

  // Copy the policy into the table.
  CopyMem (mPolicyTable + mCurrentTableUsage, NewPolicy, NewPolicy->Size);
  mCurrentTableUsage += NewPolicy->Size;
  mCurrentTableCount += 1;

  // We're done here.

  return EFI_SUCCESS;
}

/**
  This API function checks to see whether the parameters to SetVariable would
  be allowed according to the current variable policies.

  @param[in]  VariableName       Same as EFI_SET_VARIABLE.
  @param[in]  VendorGuid         Same as EFI_SET_VARIABLE.
  @param[in]  Attributes         Same as EFI_SET_VARIABLE.
  @param[in]  DataSize           Same as EFI_SET_VARIABLE.
  @param[in]  Data               Same as EFI_SET_VARIABLE.

  @retval     EFI_SUCCESS             A matching policy allows this update.
  @retval     EFI_SUCCESS             There are currently no policies that restrict this update.
  @retval     EFI_SUCCESS             The protections have been disable until the next reboot.
  @retval     EFI_WRITE_PROTECTED     Variable is currently locked.
  @retval     EFI_INVALID_PARAMETER   Attributes or size are invalid.
  @retval     EFI_ABORTED             A lock policy exists, but an error prevented evaluation.
  @retval     EFI_NOT_READY           Library has not been initialized.

**/
EFI_STATUS
EFIAPI
ValidateSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  BOOLEAN                            IsDel;
  VARIABLE_POLICY_ENTRY              *ActivePolicy;
  EFI_STATUS                         Status;
  EFI_STATUS                         ReturnStatus;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY  *StateVarPolicy;
  CHAR16                             *StateVarName;
  UINTN                              StateVarSize;
  UINT8                              StateVar;

  ReturnStatus = EFI_SUCCESS;

  if (!IsVariablePolicyLibInitialized ()) {
    ReturnStatus = EFI_NOT_READY;
    goto Exit;
  }

  // Bail if the protections are currently disabled.
  if (mProtectionDisabled) {
    ReturnStatus = EFI_SUCCESS;
    goto Exit;
  }

  // Determine whether this is a delete operation.
  // If so, it will affect which tests are applied.
  if ((DataSize == 0) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0)) {
    IsDel = TRUE;
  } else {
    IsDel = FALSE;
  }

  // Find an active policy if one exists.
  ActivePolicy = GetBestPolicyMatch (VariableName, VendorGuid, NULL);

  // If we have an active policy, check it against the incoming data.
  if (ActivePolicy != NULL) {
    //
    // Only enforce size and attribute constraints when updating data, not deleting.
    if (!IsDel) {
      // Check for size constraints.
      if (((ActivePolicy->MinSize > 0) && (DataSize < ActivePolicy->MinSize)) ||
          ((ActivePolicy->MaxSize > 0) && (DataSize > ActivePolicy->MaxSize)))
      {
        ReturnStatus = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_VERBOSE,
          "%a - Bad Size. 0x%X <> 0x%X-0x%X\n",
          __func__,
          DataSize,
          ActivePolicy->MinSize,
          ActivePolicy->MaxSize
          ));
        goto Exit;
      }

      // Check for attribute constraints.
      if (((ActivePolicy->AttributesMustHave & Attributes) != ActivePolicy->AttributesMustHave) ||
          ((ActivePolicy->AttributesCantHave & Attributes) != 0))
      {
        ReturnStatus = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_VERBOSE,
          "%a - Bad Attributes. 0x%X <> 0x%X:0x%X\n",
          __func__,
          Attributes,
          ActivePolicy->AttributesMustHave,
          ActivePolicy->AttributesCantHave
          ));
        goto Exit;
      }
    }

    //
    // Lock policy check.
    //
    // Check for immediate lock.
    if (ActivePolicy->LockPolicyType == VARIABLE_POLICY_TYPE_LOCK_NOW) {
      ReturnStatus = EFI_WRITE_PROTECTED;
      goto Exit;
      // Check for lock on create.
    } else if (ActivePolicy->LockPolicyType == VARIABLE_POLICY_TYPE_LOCK_ON_CREATE) {
      StateVarSize = 0;
      Status       = mGetVariableHelper (
                       VariableName,
                       VendorGuid,
                       NULL,
                       &StateVarSize,
                       NULL
                       );
      if (Status == EFI_BUFFER_TOO_SMALL) {
        ReturnStatus = EFI_WRITE_PROTECTED;
        goto Exit;
      }

      // Check for lock on state variable.
    } else if (ActivePolicy->LockPolicyType == VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE) {
      StateVarPolicy = (VARIABLE_LOCK_ON_VAR_STATE_POLICY *)((UINT8 *)ActivePolicy + sizeof (VARIABLE_POLICY_ENTRY));
      StateVarName   = (CHAR16 *)((UINT8 *)StateVarPolicy + sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY));
      StateVarSize   = sizeof (StateVar);
      Status         = mGetVariableHelper (
                         StateVarName,
                         &StateVarPolicy->Namespace,
                         NULL,
                         &StateVarSize,
                         &StateVar
                         );

      // If the variable was found, check the state. If matched, this variable is locked.
      if (!EFI_ERROR (Status)) {
        if (StateVar == StateVarPolicy->Value) {
          ReturnStatus = EFI_WRITE_PROTECTED;
          goto Exit;
        }

        // EFI_NOT_FOUND and EFI_BUFFER_TOO_SMALL indicate that the state doesn't match.
      } else if ((Status != EFI_NOT_FOUND) && (Status != EFI_BUFFER_TOO_SMALL)) {
        // We don't know what happened, but it isn't good.
        ReturnStatus = EFI_ABORTED;
        goto Exit;
      }
    }
  }

Exit:
  DEBUG ((DEBUG_VERBOSE, "%a - Variable (%g:%s) returning %r.\n", __func__, VendorGuid, VariableName, ReturnStatus));
  return ReturnStatus;
}

/**
  This API function disables the variable policy enforcement. If it's
  already been called once, will return EFI_ALREADY_STARTED.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   Has already been called once this boot.
  @retval     EFI_WRITE_PROTECTED   Interface has been locked until reboot.
  @retval     EFI_WRITE_PROTECTED   Interface option is disabled by platform PCD.
  @retval     EFI_NOT_READY         Library has not yet been initialized.

**/
EFI_STATUS
EFIAPI
DisableVariablePolicy (
  VOID
  )
{
  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  if (mProtectionDisabled) {
    return EFI_ALREADY_STARTED;
  }

  if (mInterfaceLocked) {
    return EFI_WRITE_PROTECTED;
  }

  if (!PcdGetBool (PcdAllowVariablePolicyEnforcementDisable)) {
    return EFI_WRITE_PROTECTED;
  }

  mProtectionDisabled = TRUE;
  return EFI_SUCCESS;
}

/**
  This API function will dump the entire contents of the variable policy table.

  Similar to GetVariable, the first call can be made with a 0 size and it will return
  the size of the buffer required to hold the entire table.

  @param[out]     Policy  Pointer to the policy buffer. Can be NULL if Size is 0.
  @param[in,out]  Size    On input, the size of the output buffer. On output, the size
                          of the data returned.

  @retval     EFI_SUCCESS             Policy data is in the output buffer and Size has been updated.
  @retval     EFI_INVALID_PARAMETER   Size is NULL, or Size is non-zero and Policy is NULL.
  @retval     EFI_BUFFER_TOO_SMALL    Size is insufficient to hold policy. Size updated with required size.
  @retval     EFI_NOT_READY           Library has not yet been initialized.

**/
EFI_STATUS
EFIAPI
DumpVariablePolicy (
  OUT     UINT8   *Policy,
  IN OUT  UINT32  *Size
  )
{
  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  // Check the parameters.
  if ((Size == NULL) || ((*Size > 0) && (Policy == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  // Make sure the size is sufficient to hold the policy table.
  if (*Size < mCurrentTableUsage) {
    *Size = mCurrentTableUsage;
    return EFI_BUFFER_TOO_SMALL;
  }

  // If we're still here, copy the table and bounce.
  CopyMem (Policy, mPolicyTable, mCurrentTableUsage);
  *Size = mCurrentTableUsage;

  return EFI_SUCCESS;
}

/**
  This function will return variable policy information for a UEFI variable with a
  registered variable policy.

  @param[in]      VariableName                          The name of the variable to use for the policy search.
  @param[in]      VendorGuid                            The vendor GUID of the variable to use for the policy search.
  @param[in,out]  VariablePolicyVariableNameBufferSize  On input, the size, in bytes, of the VariablePolicyVariableName
                                                        buffer.

                                                        On output, the size, in bytes, needed to store the variable
                                                        policy variable name.

                                                        If testing for the VariablePolicyVariableName buffer size
                                                        needed, set this value to zero so EFI_BUFFER_TOO_SMALL is
                                                        guaranteed to be returned if the variable policy variable name
                                                        is found.
  @param[out]     VariablePolicy                        Pointer to a buffer where the policy entry will be written
                                                        if found.
  @param[out]     VariablePolicyVariableName            Pointer to a buffer where the variable name used for the
                                                        variable policy will be written if a variable name is
                                                        registered.

                                                        If the variable policy is not associated with a variable name
                                                        (e.g. applied to variable vendor namespace) and this parameter
                                                        is given, this parameter will not be modified and
                                                        VariablePolicyVariableNameBufferSize will be set to zero to
                                                        indicate a name was not present.

                                                        If the pointer given is not NULL,
                                                        VariablePolicyVariableNameBufferSize must be non-NULL.

  @retval     EFI_SUCCESS             A variable policy entry was found and returned successfully.
  @retval     EFI_BAD_BUFFER_SIZE     An internal buffer size caused a calculation error.
  @retval     EFI_BUFFER_TOO_SMALL    The VariablePolicyVariableName buffer value is too small for the size needed.
                                      The buffer should now point to the size needed.
  @retval     EFI_NOT_READY           Variable policy has not yet been initialized.
  @retval     EFI_INVALID_PARAMETER   A required pointer argument passed is NULL. This will be returned if
                                      VariablePolicyVariableName is non-NULL and VariablePolicyVariableNameBufferSize
                                      is NULL.
  @retval     EFI_NOT_FOUND           A variable policy was not found for the given UEFI variable name and vendor GUID.

**/
EFI_STATUS
EFIAPI
GetVariablePolicyInfo (
  IN      CONST CHAR16           *VariableName,
  IN      CONST EFI_GUID         *VendorGuid,
  IN OUT  UINTN                  *VariablePolicyVariableNameBufferSize OPTIONAL,
  OUT     VARIABLE_POLICY_ENTRY  *VariablePolicy,
  OUT     CHAR16                 *VariablePolicyVariableName OPTIONAL
  )
{
  EFI_STATUS             Status;
  UINT8                  MatchPriority;
  UINTN                  LocalVariablePolicyVariableNameBufferSize;
  UINTN                  RequiredVariablePolicyVariableNameBufferSize;
  VARIABLE_POLICY_ENTRY  *MatchPolicy;

  Status = EFI_SUCCESS;

  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  if ((VariableName == NULL) || (VendorGuid == NULL) || (VariablePolicy == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MatchPolicy = GetBestPolicyMatch (
                  VariableName,
                  VendorGuid,
                  &MatchPriority
                  );
  if (MatchPolicy != NULL) {
    CopyMem (VariablePolicy, MatchPolicy, sizeof (*VariablePolicy));

    if (VariablePolicyVariableNameBufferSize == NULL) {
      if (VariablePolicyVariableName != NULL) {
        return EFI_INVALID_PARAMETER;
      }

      return Status;
    }

    if (MatchPolicy->Size != MatchPolicy->OffsetToName) {
      if (MatchPolicy->Size < MatchPolicy->OffsetToName) {
        ASSERT (MatchPolicy->Size > MatchPolicy->OffsetToName);
        return EFI_BAD_BUFFER_SIZE;
      }

      RequiredVariablePolicyVariableNameBufferSize = (UINTN)(MatchPolicy->Size - MatchPolicy->OffsetToName);
      ASSERT (RequiredVariablePolicyVariableNameBufferSize > 0);

      if (*VariablePolicyVariableNameBufferSize < RequiredVariablePolicyVariableNameBufferSize) {
        // Let the caller get the size needed to hold the policy variable name
        *VariablePolicyVariableNameBufferSize = RequiredVariablePolicyVariableNameBufferSize;
        return EFI_BUFFER_TOO_SMALL;
      }

      if (VariablePolicyVariableName == NULL) {
        // If the policy variable name size given is valid, then a valid policy variable name buffer should be provided
        *VariablePolicyVariableNameBufferSize = RequiredVariablePolicyVariableNameBufferSize;
        return EFI_INVALID_PARAMETER;
      }

      LocalVariablePolicyVariableNameBufferSize = *VariablePolicyVariableNameBufferSize;

      // Actual string size should match expected string size
      if (
          ((StrnLenS (GET_POLICY_NAME (MatchPolicy), RequiredVariablePolicyVariableNameBufferSize) + 1) * sizeof (CHAR16))
          != RequiredVariablePolicyVariableNameBufferSize)
      {
        ASSERT_EFI_ERROR (EFI_BAD_BUFFER_SIZE);
        return EFI_BAD_BUFFER_SIZE;
      }

      *VariablePolicyVariableNameBufferSize = RequiredVariablePolicyVariableNameBufferSize;

      Status = StrnCpyS (
                 VariablePolicyVariableName,
                 LocalVariablePolicyVariableNameBufferSize / sizeof (CHAR16),
                 GET_POLICY_NAME (MatchPolicy),
                 RequiredVariablePolicyVariableNameBufferSize / sizeof (CHAR16)
                 );
      ASSERT_EFI_ERROR (Status);
    } else {
      // A variable policy variable name is not present. Return values according to interface.
      *VariablePolicyVariableNameBufferSize = 0;
    }

    return Status;
  }

  return EFI_NOT_FOUND;
}

/**
  This function will return the Lock on Variable State policy information for the policy
  associated with the given UEFI variable.

  @param[in]      VariableName                              The name of the variable to use for the policy search.
  @param[in]      VendorGuid                                The vendor GUID of the variable to use for the policy
                                                            search.
  @param[in,out]  VariableLockPolicyVariableNameBufferSize  On input, the size, in bytes, of the
                                                            VariableLockPolicyVariableName buffer.

                                                            On output, the size, in bytes, needed to store the variable
                                                            policy variable name.

                                                            If testing for the VariableLockPolicyVariableName buffer
                                                            size needed, set this value to zero so EFI_BUFFER_TOO_SMALL
                                                            is guaranteed to be returned if the variable policy variable
                                                            name is found.
  @param[out]     VariablePolicy                            Pointer to a buffer where the policy entry will be written
                                                            if found.
  @param[out]     VariableLockPolicyVariableName            Pointer to a buffer where the variable name used for the
                                                            variable lock on variable state policy will be written if
                                                            a variable name is registered.

                                                            If the lock on variable policy is not associated with a
                                                            variable name (e.g. applied to variable vendor namespace)
                                                            and this parameter is given, this parameter will not be
                                                            modified and VariableLockPolicyVariableNameBufferSize will
                                                            be set to zero to indicate a name was not present.

                                                            If the pointer given is not NULL,
                                                            VariableLockPolicyVariableNameBufferSize must be non-NULL.

  @retval     EFI_SUCCESS             A Lock on Variable State variable policy entry was found and returned
                                      successfully.
  @retval     EFI_BAD_BUFFER_SIZE     An internal buffer size caused a calculation error.
  @retval     EFI_BUFFER_TOO_SMALL    The VariableLockPolicyVariableName buffer is too small for the size needed.
                                      The buffer should now point to the size needed.
  @retval     EFI_NOT_READY           Variable policy has not yet been initialized.
  @retval     EFI_INVALID_PARAMETER   A required pointer argument passed is NULL. This will be returned if
                                      VariableLockPolicyVariableName is non-NULL and
                                      VariableLockPolicyVariableNameBufferSize is NULL.
  @retval     EFI_NOT_FOUND           A Lock on Variable State variable policy was not found for the given UEFI
                                      variable name and vendor GUID.

**/
EFI_STATUS
EFIAPI
GetLockOnVariableStateVariablePolicyInfo (
  IN      CONST CHAR16                       *VariableName,
  IN      CONST EFI_GUID                     *VendorGuid,
  IN OUT  UINTN                              *VariableLockPolicyVariableNameBufferSize OPTIONAL,
  OUT     VARIABLE_LOCK_ON_VAR_STATE_POLICY  *VariablePolicy,
  OUT     CHAR16                             *VariableLockPolicyVariableName OPTIONAL
  )
{
  EFI_STATUS                         Status;
  UINT8                              MatchPriority;
  UINTN                              RequiredVariablePolicyVariableNameBufferSize;
  UINTN                              RequiredVariableLockPolicyVariableNameBufferSize;
  UINTN                              LocalVariablePolicyLockVariableNameBufferSize;
  UINTN                              LockOnVarStatePolicyEndOffset;
  CHAR16                             *LocalVariableLockPolicyVariableName;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY  *LocalLockOnVarStatePolicy;
  VARIABLE_POLICY_ENTRY              *MatchPolicy;

  Status = EFI_SUCCESS;

  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  if ((VariableName == NULL) || (VendorGuid == NULL) || (VariablePolicy == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MatchPolicy = GetBestPolicyMatch (
                  VariableName,
                  VendorGuid,
                  &MatchPriority
                  );
  if (MatchPolicy != NULL) {
    if (MatchPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE) {
      return EFI_NOT_FOUND;
    }

    Status = SafeUintnAdd (
               sizeof (VARIABLE_POLICY_ENTRY),
               sizeof (VARIABLE_LOCK_ON_VAR_STATE_POLICY),
               &LockOnVarStatePolicyEndOffset
               );
    if (EFI_ERROR (Status) || (LockOnVarStatePolicyEndOffset > (UINTN)MatchPolicy->Size)) {
      return EFI_BAD_BUFFER_SIZE;
    }

    LocalLockOnVarStatePolicy = (VARIABLE_LOCK_ON_VAR_STATE_POLICY *)(MatchPolicy + 1);
    CopyMem (VariablePolicy, LocalLockOnVarStatePolicy, sizeof (*LocalLockOnVarStatePolicy));

    if (VariableLockPolicyVariableNameBufferSize == NULL) {
      if (VariableLockPolicyVariableName != NULL) {
        return EFI_INVALID_PARAMETER;
      }

      return Status;
    }

    // The name offset should be less than or equal to the total policy size.
    if (MatchPolicy->Size < MatchPolicy->OffsetToName) {
      return EFI_BAD_BUFFER_SIZE;
    }

    RequiredVariablePolicyVariableNameBufferSize     = (UINTN)(MatchPolicy->Size - MatchPolicy->OffsetToName);
    RequiredVariableLockPolicyVariableNameBufferSize =  MatchPolicy->Size -
                                                       (LockOnVarStatePolicyEndOffset + RequiredVariablePolicyVariableNameBufferSize);

    LocalVariablePolicyLockVariableNameBufferSize = *VariableLockPolicyVariableNameBufferSize;
    *VariableLockPolicyVariableNameBufferSize     = RequiredVariableLockPolicyVariableNameBufferSize;

    if (LocalVariablePolicyLockVariableNameBufferSize < RequiredVariableLockPolicyVariableNameBufferSize) {
      // Let the caller get the size needed to hold the policy variable name
      return EFI_BUFFER_TOO_SMALL;
    }

    if (VariableLockPolicyVariableName == NULL) {
      // If the policy variable name size given is valid, then a valid policy variable name buffer should be provided
      return EFI_INVALID_PARAMETER;
    }

    if (RequiredVariableLockPolicyVariableNameBufferSize == 0) {
      return Status;
    }

    LocalVariableLockPolicyVariableName       = (CHAR16 *)((UINT8 *)LocalLockOnVarStatePolicy + sizeof (*LocalLockOnVarStatePolicy));
    *VariableLockPolicyVariableNameBufferSize = RequiredVariableLockPolicyVariableNameBufferSize;

    // Actual string size should match expected string size (if a variable name is present)
    if (
        (RequiredVariablePolicyVariableNameBufferSize > 0) &&
        (((StrnLenS (GET_POLICY_NAME (MatchPolicy), RequiredVariablePolicyVariableNameBufferSize) + 1) * sizeof (CHAR16)) !=
         RequiredVariablePolicyVariableNameBufferSize))
    {
      ASSERT_EFI_ERROR (EFI_BAD_BUFFER_SIZE);
      return EFI_BAD_BUFFER_SIZE;
    }

    // Actual string size should match expected string size (if here, variable lock variable name is present)
    if (
        ((StrnLenS (LocalVariableLockPolicyVariableName, RequiredVariableLockPolicyVariableNameBufferSize) + 1) * sizeof (CHAR16)) !=
        RequiredVariableLockPolicyVariableNameBufferSize)
    {
      ASSERT_EFI_ERROR (EFI_BAD_BUFFER_SIZE);
      return EFI_BAD_BUFFER_SIZE;
    }

    Status =  StrnCpyS (
                VariableLockPolicyVariableName,
                LocalVariablePolicyLockVariableNameBufferSize / sizeof (CHAR16),
                LocalVariableLockPolicyVariableName,
                RequiredVariableLockPolicyVariableNameBufferSize / sizeof (CHAR16)
                );
    ASSERT_EFI_ERROR (Status);

    return Status;
  }

  return EFI_NOT_FOUND;
}

/**
  This API function returns whether or not the policy engine is
  currently being enforced.

  @retval     TRUE
  @retval     FALSE
  @retval     FALSE         Library has not yet been initialized.

**/
BOOLEAN
EFIAPI
IsVariablePolicyEnabled (
  VOID
  )
{
  if (!IsVariablePolicyLibInitialized ()) {
    return FALSE;
  }

  return !mProtectionDisabled;
}

/**
  This API function locks the interface so that no more policy updates
  can be performed or changes made to the enforcement until the next boot.

  @retval     EFI_SUCCESS
  @retval     EFI_NOT_READY   Library has not yet been initialized.

**/
EFI_STATUS
EFIAPI
LockVariablePolicy (
  VOID
  )
{
  if (!IsVariablePolicyLibInitialized ()) {
    return EFI_NOT_READY;
  }

  if (mInterfaceLocked) {
    return EFI_WRITE_PROTECTED;
  }

  mInterfaceLocked = TRUE;
  return EFI_SUCCESS;
}

/**
  This API function returns whether or not the policy interface is locked
  for the remainder of the boot.

  @retval     TRUE
  @retval     FALSE
  @retval     FALSE         Library has not yet been initialized.

**/
BOOLEAN
EFIAPI
IsVariablePolicyInterfaceLocked (
  VOID
  )
{
  if (!IsVariablePolicyLibInitialized ()) {
    return FALSE;
  }

  return mInterfaceLocked;
}

/**
  This helper function initializes the library and sets
  up any required internal structures or handlers.

  Also registers the internal pointer for the GetVariable helper.

  @param[in]  GetVariableHelper A function pointer matching the EFI_GET_VARIABLE prototype that will be used to
                  check policy criteria that involve the existence of other variables.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   The initialize function has been called more than once without a call to
                                    deinitialize.

**/
EFI_STATUS
EFIAPI
InitVariablePolicyLib (
  IN  EFI_GET_VARIABLE  GetVariableHelper
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mGetVariableHelper != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (!EFI_ERROR (Status)) {
    Status = VariablePolicyExtraInit ();
  }

  if (!EFI_ERROR (Status)) {
    // Save an internal pointer to the GetVariableHelper.
    mGetVariableHelper = GetVariableHelper;

    // Initialize the global state.
    mInterfaceLocked    = FALSE;
    mProtectionDisabled = FALSE;
    mPolicyTable        = NULL;
    mCurrentTableSize   = 0;
    mCurrentTableUsage  = 0;
    mCurrentTableCount  = 0;
  }

  return Status;
}

/**
  This helper function returns whether or not the library is currently initialized.

  @retval     TRUE
  @retval     FALSE

**/
BOOLEAN
EFIAPI
IsVariablePolicyLibInitialized (
  VOID
  )
{
  return (mGetVariableHelper != NULL);
}

/**
  This helper function tears down  the library.

  Should generally only be used for test harnesses.

  @retval     EFI_SUCCESS
  @retval     EFI_NOT_READY     Deinitialize was called without first calling initialize.

**/
EFI_STATUS
EFIAPI
DeinitVariablePolicyLib (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mGetVariableHelper == NULL) {
    return EFI_NOT_READY;
  }

  if (!EFI_ERROR (Status)) {
    Status = VariablePolicyExtraDeinit ();
  }

  if (!EFI_ERROR (Status)) {
    mGetVariableHelper  = NULL;
    mInterfaceLocked    = FALSE;
    mProtectionDisabled = FALSE;
    mCurrentTableSize   = 0;
    mCurrentTableUsage  = 0;
    mCurrentTableCount  = 0;

    if (mPolicyTable != NULL) {
      FreePool (mPolicyTable);
      mPolicyTable = NULL;
    }
  }

  return Status;
}
