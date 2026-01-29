/** @file -- VariablePolicyLib.h
Business logic for Variable Policy enforcement.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_POLICY_LIB_H_
#define _VARIABLE_POLICY_LIB_H_

#include <Protocol/VariablePolicy.h>

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
  );

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
  );

/**
  This API function disables the variable policy enforcement. If it's
  already been called once, will return EFI_ALREADY_STARTED.

  @retval     EFI_SUCCESS
  @retval     EFI_ALREADY_STARTED   Has already been called once this boot.
  @retval     EFI_WRITE_PROTECTED   Interface has been locked until reboot.
  @retval     EFI_WRITE_PROTECTED   Interface option is disabled by platform PCD.
  @retval     EFI_NOT_READY   Library has not yet been initialized.

**/
EFI_STATUS
EFIAPI
DisableVariablePolicy (
  VOID
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  This helper function returns whether or not the library is currently initialized.

  @retval     TRUE
  @retval     FALSE

**/
BOOLEAN
EFIAPI
IsVariablePolicyLibInitialized (
  VOID
  );

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
  );

#endif // _VARIABLE_POLICY_LIB_H_
