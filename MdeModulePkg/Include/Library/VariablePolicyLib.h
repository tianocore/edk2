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
