/** @file -- VariablePolicyHelperLib.c
This library contains helper functions for marshalling and registering
new policies with the VariablePolicy infrastructure.

This library is currently written against VariablePolicy revision 0x00010000.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariablePolicy.h>

/**
  This internal helper function populates the header structure,
  all common fields, and takes care of fix-ups.

  NOTE: Only use this internally. Assumes correctly-sized buffers.

  @param[out] EntPtr      Pointer to the buffer to be populated.
  @param[in]  Namespace   Pointer to an EFI_GUID for the target variable namespace that this policy will protect.
  @param[in]  MinSize     MinSize for the VariablePolicy.
  @param[in]  MaxSize     MaxSize for the VariablePolicy.
  @param[in]  AttributesMustHave    AttributesMustHave for the VariablePolicy.
  @param[in]  AttributesCantHave    AttributesCantHave for the VariablePolicy.
  @param[in]  LockPolicyType        LockPolicyType for the VariablePolicy.

**/
STATIC
VOID
PopulateCommonData (
  OUT VARIABLE_POLICY_ENTRY   *EntPtr,
  IN CONST  EFI_GUID          *Namespace,
  IN        UINT32            MinSize,
  IN        UINT32            MaxSize,
  IN        UINT32            AttributesMustHave,
  IN        UINT32            AttributesCantHave,
  IN        UINT8             LockPolicyType
  )
{
  EntPtr->Version             = VARIABLE_POLICY_ENTRY_REVISION;
  CopyGuid( &EntPtr->Namespace, Namespace );
  EntPtr->MinSize             = MinSize;
  EntPtr->MaxSize             = MaxSize;
  EntPtr->AttributesMustHave  = AttributesMustHave;
  EntPtr->AttributesCantHave  = AttributesCantHave;
  EntPtr->LockPolicyType      = LockPolicyType;

  // NOTE: As a heler, fix up MaxSize for compatibility with the old model.
  if (EntPtr->MaxSize == 0) {
    EntPtr->MaxSize = VARIABLE_POLICY_NO_MAX_SIZE;
  }

  return;
}


/**
  This helper function will allocate and populate a new VariablePolicy
  structure for a policy that does not contain any sub-structures (such as
  VARIABLE_LOCK_ON_VAR_STATE_POLICY).

  NOTE: Caller will need to free structure once finished.

  @param[in]  Namespace   Pointer to an EFI_GUID for the target variable namespace that this policy will protect.
  @param[in]  Name        [Optional] If provided, a pointer to the CHAR16 array for the target variable name.
                          Otherwise, will create a policy that targets an entire namespace.
  @param[in]  MinSize     MinSize for the VariablePolicy.
  @param[in]  MaxSize     MaxSize for the VariablePolicy.
  @param[in]  AttributesMustHave    AttributesMustHave for the VariablePolicy.
  @param[in]  AttributesCantHave    AttributesCantHave for the VariablePolicy.
  @param[in]  LockPolicyType        LockPolicyType for the VariablePolicy.
  @param[out] NewEntry    If successful, will be set to a pointer to the allocated buffer containing the
                          new policy.

  @retval     EFI_SUCCESS             Operation completed successfully and structure is populated.
  @retval     EFI_INVALID_PARAMETER   Namespace is NULL.
  @retval     EFI_INVALID_PARAMETER   LockPolicyType is invalid for a basic structure.
  @retval     EFI_BUFFER_TOO_SMALL    Finished structure would not fit in UINT16 size.
  @retval     EFI_OUT_OF_RESOURCES    Could not allocate sufficient space for structure.

**/
EFI_STATUS
EFIAPI
CreateBasicVariablePolicy (
  IN CONST  EFI_GUID          *Namespace,
  IN CONST  CHAR16            *Name OPTIONAL,
  IN        UINT32            MinSize,
  IN        UINT32            MaxSize,
  IN        UINT32            AttributesMustHave,
  IN        UINT32            AttributesCantHave,
  IN        UINT8             LockPolicyType,
  OUT VARIABLE_POLICY_ENTRY   **NewEntry
  )
{
  UINTN                   TotalSize;
  UINTN                   NameSize;
  VARIABLE_POLICY_ENTRY   *EntPtr;
  CHAR16                  *CopyName;

  // Check some initial invalid parameters for this function.
  if (Namespace == NULL || NewEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (LockPolicyType != VARIABLE_POLICY_TYPE_NO_LOCK &&
      LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_NOW &&
      LockPolicyType != VARIABLE_POLICY_TYPE_LOCK_ON_CREATE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set NameSize to suppress incorrect compiler/analyzer warnings
  //
  NameSize  = 0;

  // Now we've gotta determine the total size of the buffer required for
  // the VariablePolicy structure.
  TotalSize = sizeof( VARIABLE_POLICY_ENTRY );
  if (Name != NULL) {
    NameSize = StrnSizeS( Name, MAX_UINT16 );
    TotalSize += NameSize;
  }
  // Make sure the size fits within a VARIABLE_POLICY_ENTRY.Size.
  ASSERT( TotalSize <= MAX_UINT16 );
  if (TotalSize > MAX_UINT16) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Allocate a buffer to hold all the data. We're on the home stretch.
  *NewEntry = AllocatePool( TotalSize );
  if (*NewEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // If we're still here, we're basically done.
  // Copy the data and GET... OUT....
  EntPtr = *NewEntry;
  PopulateCommonData ( EntPtr,
                       Namespace,
                       MinSize,
                       MaxSize,
                       AttributesMustHave,
                       AttributesCantHave,
                       LockPolicyType );
  EntPtr->Size                = (UINT16)TotalSize;      // This is safe because we've already checked.
  EntPtr->OffsetToName        = sizeof(VARIABLE_POLICY_ENTRY);
  if (Name != NULL) {
    CopyName = (CHAR16*)((UINT8*)EntPtr + EntPtr->OffsetToName);
    CopyMem( CopyName, Name, NameSize );
  }

  return EFI_SUCCESS;
}


/**
  This helper function will allocate and populate a new VariablePolicy
  structure for a policy with a lock type of VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE.

  NOTE: Caller will need to free structure once finished.

  @param[in]  Namespace   Pointer to an EFI_GUID for the target variable namespace that this policy will protect.
  @param[in]  Name        [Optional] If provided, a pointer to the CHAR16 array for the target variable name.
                          Otherwise, will create a policy that targets an entire namespace.
  @param[in]  MinSize     MinSize for the VariablePolicy.
  @param[in]  MaxSize     MaxSize for the VariablePolicy.
  @param[in]  AttributesMustHave    AttributesMustHave for the VariablePolicy.
  @param[in]  AttributesCantHave    AttributesCantHave for the VariablePolicy.
  @param[in]  VarStateNamespace     Pointer to the EFI_GUID for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Namespace.
  @param[in]  VarStateValue         Value for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Value.
  @param[in]  VarStateName          Pointer to the CHAR16 array for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Name.
  @param[out] NewEntry    If successful, will be set to a pointer to the allocated buffer containing the
                          new policy.

  @retval     EFI_SUCCESS             Operation completed successfully and structure is populated.
  @retval     EFI_INVALID_PARAMETER   Namespace, VarStateNamespace, VarStateName is NULL.
  @retval     EFI_BUFFER_TOO_SMALL    Finished structure would not fit in UINT16 size.
  @retval     EFI_OUT_OF_RESOURCES    Could not allocate sufficient space for structure.

**/
EFI_STATUS
EFIAPI
CreateVarStateVariablePolicy (
  IN CONST  EFI_GUID          *Namespace,
  IN CONST  CHAR16            *Name OPTIONAL,
  IN        UINT32            MinSize,
  IN        UINT32            MaxSize,
  IN        UINT32            AttributesMustHave,
  IN        UINT32            AttributesCantHave,
  IN CONST  EFI_GUID          *VarStateNamespace,
  IN        UINT8             VarStateValue,
  IN CONST  CHAR16            *VarStateName,
  OUT VARIABLE_POLICY_ENTRY   **NewEntry
  )
{
  UINTN                   TotalSize;
  UINTN                   NameSize;
  UINTN                   VarStateNameSize;
  VARIABLE_POLICY_ENTRY   *EntPtr;
  CHAR16                  *CopyName;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY *CopyPolicy;

  // Check some initial invalid parameters for this function.
  if (Namespace == NULL || VarStateNamespace == NULL ||
      VarStateName == NULL || NewEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Now we've gotta determine the total size of the buffer required for
  // the VariablePolicy structure.
  VarStateNameSize = StrnSizeS( VarStateName, MAX_UINT16 );
  TotalSize = sizeof( VARIABLE_POLICY_ENTRY ) +
                sizeof(VARIABLE_LOCK_ON_VAR_STATE_POLICY) +
                VarStateNameSize;
  if (Name != NULL) {
    NameSize = StrnSizeS( Name, MAX_UINT16 );
    TotalSize += NameSize;
  }
  // Make sure the size fits within a VARIABLE_POLICY_ENTRY.Size.
  ASSERT( TotalSize <= MAX_UINT16 );
  if (TotalSize > MAX_UINT16) {
    return EFI_BUFFER_TOO_SMALL;
  }

  // Allocate a buffer to hold all the data. We're on the home stretch.
  *NewEntry = AllocatePool( TotalSize );
  if (*NewEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // If we're still here, we're basically done.
  // Copy the data and GET... OUT....
  EntPtr = *NewEntry;
  PopulateCommonData ( EntPtr,
                       Namespace,
                       MinSize,
                       MaxSize,
                       AttributesMustHave,
                       AttributesCantHave,
                       VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE );
  EntPtr->Size                = (UINT16)TotalSize;      // This is safe because we've already checked.
  EntPtr->OffsetToName        = sizeof(VARIABLE_POLICY_ENTRY) +
                                sizeof(VARIABLE_LOCK_ON_VAR_STATE_POLICY) +
                                (UINT16)VarStateNameSize;

  CopyPolicy = (VARIABLE_LOCK_ON_VAR_STATE_POLICY*)((UINT8*)EntPtr + sizeof(VARIABLE_POLICY_ENTRY));
  CopyName = (CHAR16*)((UINT8*)CopyPolicy + sizeof(VARIABLE_LOCK_ON_VAR_STATE_POLICY));
  CopyGuid( &CopyPolicy->Namespace, VarStateNamespace );
  CopyPolicy->Value = VarStateValue;
  CopyMem( CopyName, VarStateName, VarStateNameSize );

  if (Name != NULL) {
    CopyName = (CHAR16*)((UINT8*)EntPtr + EntPtr->OffsetToName);
    CopyMem( CopyName, Name, NameSize );
  }

  return EFI_SUCCESS;
}


/**
  This helper function does everything that CreateBasicVariablePolicy() does, but also
  uses the passed in protocol to register the policy with the infrastructure.
  Does not return a buffer, does not require the caller to free anything.

  @param[in]  VariablePolicy  Pointer to a valid instance of the VariablePolicy protocol.
  @param[in]  Namespace   Pointer to an EFI_GUID for the target variable namespace that this policy will protect.
  @param[in]  Name        [Optional] If provided, a pointer to the CHAR16 array for the target variable name.
                          Otherwise, will create a policy that targets an entire namespace.
  @param[in]  MinSize     MinSize for the VariablePolicy.
  @param[in]  MaxSize     MaxSize for the VariablePolicy.
  @param[in]  AttributesMustHave    AttributesMustHave for the VariablePolicy.
  @param[in]  AttributesCantHave    AttributesCantHave for the VariablePolicy.
  @param[in]  LockPolicyType        LockPolicyType for the VariablePolicy.

  @retval     EFI_INVALID_PARAMETER VariablePolicy pointer is NULL.
  @retval     EFI_STATUS            Status returned by CreateBasicVariablePolicy() or RegisterVariablePolicy().

**/
EFI_STATUS
EFIAPI
RegisterBasicVariablePolicy (
  IN        EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy,
  IN CONST  EFI_GUID                        *Namespace,
  IN CONST  CHAR16                          *Name OPTIONAL,
  IN        UINT32                          MinSize,
  IN        UINT32                          MaxSize,
  IN        UINT32                          AttributesMustHave,
  IN        UINT32                          AttributesCantHave,
  IN        UINT8                           LockPolicyType
  )
{
  VARIABLE_POLICY_ENTRY   *NewEntry;
  EFI_STATUS              Status;

  // Check the simple things.
  if (VariablePolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Create the new entry and make sure that everything worked.
  NewEntry = NULL;
  Status = CreateBasicVariablePolicy( Namespace,
                                      Name,
                                      MinSize,
                                      MaxSize,
                                      AttributesMustHave,
                                      AttributesCantHave,
                                      LockPolicyType,
                                      &NewEntry );

  // If that was successful, attempt to register the new policy.
  if (!EFI_ERROR( Status )) {
    Status = VariablePolicy->RegisterVariablePolicy( NewEntry );
  }

  // If we allocated the buffer, free the buffer.
  if (NewEntry != NULL) {
    FreePool( NewEntry );
  }

  return Status;
}


/**
  This helper function does everything that CreateBasicVariablePolicy() does, but also
  uses the passed in protocol to register the policy with the infrastructure.
  Does not return a buffer, does not require the caller to free anything.

  @param[in]  VariablePolicy  Pointer to a valid instance of the VariablePolicy protocol.
  @param[in]  Namespace   Pointer to an EFI_GUID for the target variable namespace that this policy will protect.
  @param[in]  Name        [Optional] If provided, a pointer to the CHAR16 array for the target variable name.
                          Otherwise, will create a policy that targets an entire namespace.
  @param[in]  MinSize     MinSize for the VariablePolicy.
  @param[in]  MaxSize     MaxSize for the VariablePolicy.
  @param[in]  AttributesMustHave    AttributesMustHave for the VariablePolicy.
  @param[in]  AttributesCantHave    AttributesCantHave for the VariablePolicy.
  @param[in]  VarStateNamespace     Pointer to the EFI_GUID for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Namespace.
  @param[in]  VarStateName          Pointer to the CHAR16 array for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Name.
  @param[in]  VarStateValue         Value for the VARIABLE_LOCK_ON_VAR_STATE_POLICY.Value.

  @retval     EFI_INVALID_PARAMETER VariablePolicy pointer is NULL.
  @retval     EFI_STATUS    Status returned by CreateBasicVariablePolicy() or RegisterVariablePolicy().

**/
EFI_STATUS
EFIAPI
RegisterVarStateVariablePolicy (
  IN        EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy,
  IN CONST  EFI_GUID                        *Namespace,
  IN CONST  CHAR16                          *Name OPTIONAL,
  IN        UINT32                          MinSize,
  IN        UINT32                          MaxSize,
  IN        UINT32                          AttributesMustHave,
  IN        UINT32                          AttributesCantHave,
  IN CONST  EFI_GUID                        *VarStateNamespace,
  IN CONST  CHAR16                          *VarStateName,
  IN        UINT8                           VarStateValue
  )
{
  VARIABLE_POLICY_ENTRY   *NewEntry;
  EFI_STATUS              Status;

  // Check the simple things.
  if (VariablePolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Create the new entry and make sure that everything worked.
  NewEntry = NULL;
  Status = CreateVarStateVariablePolicy( Namespace,
                                         Name,
                                         MinSize,
                                         MaxSize,
                                         AttributesMustHave,
                                         AttributesCantHave,
                                         VarStateNamespace,
                                         VarStateValue,
                                         VarStateName,
                                         &NewEntry );

  // If that was successful, attempt to register the new policy.
  if (!EFI_ERROR( Status )) {
    Status = VariablePolicy->RegisterVariablePolicy( NewEntry );
  }

  // If we allocated the buffer, free the buffer.
  if (NewEntry != NULL) {
    FreePool( NewEntry );
  }

  return Status;
}
