/** @file -- VariablePolicyHelperLib.h
This library contains helper functions for marshalling and registering
new policies with the VariablePolicy infrastructure.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_VARIABLE_POLICY_HELPER_LIB_H_
#define _EDKII_VARIABLE_POLICY_HELPER_LIB_H_

#include <Protocol/VariablePolicy.h>

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
  IN CONST  EFI_GUID         *Namespace,
  IN CONST  CHAR16           *Name OPTIONAL,
  IN        UINT32           MinSize,
  IN        UINT32           MaxSize,
  IN        UINT32           AttributesMustHave,
  IN        UINT32           AttributesCantHave,
  IN        UINT8            LockPolicyType,
  OUT VARIABLE_POLICY_ENTRY  **NewEntry
  );

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
  IN CONST  EFI_GUID         *Namespace,
  IN CONST  CHAR16           *Name OPTIONAL,
  IN        UINT32           MinSize,
  IN        UINT32           MaxSize,
  IN        UINT32           AttributesMustHave,
  IN        UINT32           AttributesCantHave,
  IN CONST  EFI_GUID         *VarStateNamespace,
  IN        UINT8            VarStateValue,
  IN CONST  CHAR16           *VarStateName,
  OUT VARIABLE_POLICY_ENTRY  **NewEntry
  );

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
  );

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
  );

#endif // _EDKII_VARIABLE_POLICY_HELPER_LIB_H_
