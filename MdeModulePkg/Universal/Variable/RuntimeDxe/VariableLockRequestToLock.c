/** @file
  Temporary location of the RequestToLock shim code while projects
  are moved to VariablePolicy. Should be removed when deprecated.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/VariablePolicyLib.h>
#include <Library/VariablePolicyHelperLib.h>
#include <Protocol/VariableLock.h>

//
// NOTE: DO NOT USE THESE MACROS on any structure that has not been validated.
//       Current table data has already been sanitized.
//
#define GET_NEXT_POLICY(CurPolicy)  (VARIABLE_POLICY_ENTRY*)((UINT8*)CurPolicy + CurPolicy->Size)
#define GET_POLICY_NAME(CurPolicy)  (CHAR16*)((UINTN)CurPolicy + CurPolicy->OffsetToName)

#define MATCH_PRIORITY_EXACT  0
#define MATCH_PRIORITY_MIN    MAX_UINT8

/**
  This helper function evaluates a policy and determines whether it matches the
  target variable. If matched, will also return a value corresponding to the
  priority of the match.

  The rules for "best match" are listed in the Variable Policy Spec.
  Perfect name matches will return 0.
  Single wildcard characters will return the number of wildcard characters.
  Full namespaces will return MAX_UINT8.

  @param[in]  EvalEntry      Pointer to the policy entry being evaluated.
  @param[in]  VariableName   Same as EFI_SET_VARIABLE.
  @param[in]  VendorGuid     Same as EFI_SET_VARIABLE.
  @param[out] MatchPriority  [Optional] On finding a match, this value contains
                             the priority of the match. Lower number == higher
                             priority. Only valid if a match found.

  @retval  TRUE   Current entry matches the target variable.
  @retval  FALSE  Current entry does not match at all.

**/
STATIC
BOOLEAN
EvaluatePolicyMatch (
  IN CONST VARIABLE_POLICY_ENTRY  *EvalEntry,
  IN CONST CHAR16                 *VariableName,
  IN CONST EFI_GUID               *VendorGuid,
  OUT UINT8                       *MatchPriority  OPTIONAL
  )
{
  BOOLEAN  Result;
  CHAR16   *PolicyName;
  UINT8    CalculatedPriority;
  UINTN    Index;

  Result = FALSE;
  CalculatedPriority = MATCH_PRIORITY_EXACT;

  //
  // Step 1: If the GUID doesn't match, we're done. No need to evaluate anything else.
  //
  if (!CompareGuid (&EvalEntry->Namespace, VendorGuid)) {
    goto Exit;
  }

  //
  // If the GUID matches, check to see whether there is a Name associated
  // with the policy. If not, this policy matches the entire namespace.
  // Missing Name is indicated by size being equal to name.
  //
  if (EvalEntry->Size == EvalEntry->OffsetToName) {
    CalculatedPriority = MATCH_PRIORITY_MIN;
    Result = TRUE;
    goto Exit;
  }

  //
  // Now that we know the name exists, get it.
  //
  PolicyName = GET_POLICY_NAME (EvalEntry);

  //
  // Evaluate the name against the policy name and check for a match.
  // Account for any wildcards.
  //
  Index = 0;
  Result = TRUE;
  //
  // Keep going until the end of both strings.
  //
  while (PolicyName[Index] != CHAR_NULL || VariableName[Index] != CHAR_NULL) {
    //
    // If we don't have a match...
    //
    if (PolicyName[Index] != VariableName[Index] || PolicyName[Index] == '#') {
      //
      // If this is a numerical wildcard, we can consider it a match if we alter
      // the priority.
      //
      if (PolicyName[Index] == L'#' &&
            ((L'0' <= VariableName[Index] && VariableName[Index] <= L'9') ||
             (L'A' <= VariableName[Index] && VariableName[Index] <= L'F') ||
             (L'a' <= VariableName[Index] && VariableName[Index] <= L'f'))) {
        if (CalculatedPriority < MATCH_PRIORITY_MIN) {
          CalculatedPriority++;
        }
      //
      // Otherwise, not a match.
      //
      } else {
        Result = FALSE;
        goto Exit;
      }
    }
    Index++;
  }

Exit:
  if (Result && MatchPriority != NULL) {
    *MatchPriority = CalculatedPriority;
  }
  return Result;
}

/**
  This helper function walks the current policy table and returns a pointer
  to the best match, if any are found. Leverages EvaluatePolicyMatch() to
  determine "best".

  @param[in]  PolicyTable      Pointer to current policy table.
  @param[in]  PolicyTableSize  Size of current policy table.
  @param[in]  VariableName     Same as EFI_SET_VARIABLE.
  @param[in]  VendorGuid       Same as EFI_SET_VARIABLE.
  @param[out] ReturnPriority   [Optional] If pointer is provided, return the
                               priority of the match. Same as EvaluatePolicyMatch().
                               Only valid if a match is returned.

  @retval     VARIABLE_POLICY_ENTRY*    Best match that was found.
  @retval     NULL                      No match was found.

**/
STATIC
VARIABLE_POLICY_ENTRY*
GetBestPolicyMatch (
  IN UINT8           *PolicyTable,
  IN UINT32          PolicyTableSize,
  IN CONST CHAR16    *VariableName,
  IN CONST EFI_GUID  *VendorGuid,
  OUT UINT8          *ReturnPriority  OPTIONAL
  )
{
  VARIABLE_POLICY_ENTRY  *BestResult;
  VARIABLE_POLICY_ENTRY  *CurrentEntry;
  UINT8                  MatchPriority;
  UINT8                  CurrentPriority;

  BestResult = NULL;
  MatchPriority = MATCH_PRIORITY_EXACT;

  //
  // Walk all entries in the table, looking for matches.
  //
  CurrentEntry = (VARIABLE_POLICY_ENTRY*)PolicyTable;
  while ((UINTN)CurrentEntry < (UINTN)((UINT8*)PolicyTable + PolicyTableSize)) {
    //
    // Check for a match.
    //
    if (EvaluatePolicyMatch (CurrentEntry, VariableName, VendorGuid, &CurrentPriority)) {
      //
      // If match is better, take it.
      //
      if (BestResult == NULL || CurrentPriority < MatchPriority) {
        BestResult = CurrentEntry;
        MatchPriority = CurrentPriority;
      }

      //
      // If you've hit the highest-priority match, can exit now.
      //
      if (MatchPriority == 0) {
        break;
      }
    }

    //
    // If we're still in the loop, move to the next entry.
    //
    CurrentEntry = GET_NEXT_POLICY (CurrentEntry);
  }

  //
  // If a return priority was requested, return it.
  //
  if (ReturnPriority != NULL) {
    *ReturnPriority = MatchPriority;
  }

  return BestResult;
}

/**
  This helper function will dump and walk the current policy tables to determine
  whether a matching policy already exists that satisfies the lock request.

  @param[in] VariableName  A pointer to the variable name that is being searched.
  @param[in] VendorGuid    A pointer to the vendor GUID that is being searched.

  @retval  TRUE   We can safely assume this variable is locked.
  @retval  FALSE  An error has occurred or we cannot prove that the variable is
                  locked.

**/
STATIC
BOOLEAN
IsVariableAlreadyLocked (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS             Status;
  UINT8                  *PolicyTable;
  UINT32                 PolicyTableSize;
  BOOLEAN                Result;
  VARIABLE_POLICY_ENTRY  *MatchPolicy;
  UINT8                  MatchPriority;

  Result = TRUE;

  //
  // First, we need to dump the existing policy table.
  //
  PolicyTableSize = 0;
  PolicyTable = NULL;
  Status = DumpVariablePolicy (PolicyTable, &PolicyTableSize);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to determine policy table size! %r\n", __FUNCTION__, Status));
    return FALSE;
  }
  PolicyTable = AllocateZeroPool (PolicyTableSize);
  if (PolicyTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocated space for policy table! 0x%X\n", __FUNCTION__, PolicyTableSize));
    return FALSE;
  }
  Status = DumpVariablePolicy (PolicyTable, &PolicyTableSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to dump policy table! %r\n", __FUNCTION__, Status));
    Result = FALSE;
    goto Exit;
  }

  //
  // Now we need to walk the table looking for a match.
  //
  MatchPolicy = GetBestPolicyMatch (
                  PolicyTable,
                  PolicyTableSize,
                  VariableName,
                  VendorGuid,
                  &MatchPriority
                  );
  if (MatchPolicy != NULL && MatchPriority != MATCH_PRIORITY_EXACT) {
    DEBUG ((DEBUG_ERROR, "%a - We would not have expected a non-exact match! %d\n", __FUNCTION__, MatchPriority));
    Result = FALSE;
    goto Exit;
  }

  //
  // Now we can check to see whether this variable is currently locked.
  //
  if (MatchPolicy->LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_NOW) {
    DEBUG ((DEBUG_INFO, "%a - Policy may not lock variable! %d\n", __FUNCTION__, MatchPolicy->LockPolicyType));
    Result = FALSE;
    goto Exit;
  }

Exit:
  if (PolicyTable != NULL) {
    FreePool (PolicyTable);
  }

  return Result;
}

/**
  DEPRECATED. THIS IS ONLY HERE AS A CONVENIENCE WHILE PORTING.
  Mark a variable that will become read-only after leaving the DXE phase of
  execution. Write request coming from SMM environment through
  EFI_SMM_VARIABLE_PROTOCOL is allowed.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made
                           read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made
                           read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and
                                the VendorGuid was marked as pending to be
                                read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or
                                EFI_EVENT_GROUP_READY_TO_BOOT has already been
                                signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock
                                request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL  *This,
  IN CHAR16                              *VariableName,
  IN EFI_GUID                            *VendorGuid
  )
{
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *NewPolicy;

  DEBUG ((DEBUG_ERROR, "!!! DEPRECATED INTERFACE !!! %a() will go away soon!\n", __FUNCTION__));
  DEBUG ((DEBUG_ERROR, "!!! DEPRECATED INTERFACE !!! Please move to use Variable Policy!\n"));
  DEBUG ((DEBUG_ERROR, "!!! DEPRECATED INTERFACE !!! Variable: %g %s\n", VendorGuid, VariableName));

  NewPolicy = NULL;
  Status = CreateBasicVariablePolicy(
             VendorGuid,
             VariableName,
             VARIABLE_POLICY_NO_MIN_SIZE,
             VARIABLE_POLICY_NO_MAX_SIZE,
             VARIABLE_POLICY_NO_MUST_ATTR,
             VARIABLE_POLICY_NO_CANT_ATTR,
             VARIABLE_POLICY_TYPE_LOCK_NOW,
             &NewPolicy
             );
  if (!EFI_ERROR( Status )) {
    Status = RegisterVariablePolicy (NewPolicy);

    //
    // If the error returned is EFI_ALREADY_STARTED, we need to check the current database for
    // the variable and see whether it's locked. If it's locked, we're still fine.
    //
    if (Status == EFI_ALREADY_STARTED) {
      if (IsVariableAlreadyLocked (VariableName, VendorGuid)) {
        Status = EFI_SUCCESS;
      }
    }
  }
  if (EFI_ERROR (Status)) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to lock variable %s! %r\n", __FUNCTION__, VariableName, Status ));
  }
  if (NewPolicy != NULL) {
    FreePool( NewPolicy );
  }

  return Status;
}
