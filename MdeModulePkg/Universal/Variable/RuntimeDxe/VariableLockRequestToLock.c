/** @file -- VariableLockRequestToLock.c
Temporary location of the RequestToLock shim code while
projects are moved to VariablePolicy. Should be removed when deprecated.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/VariableLock.h>

#include <Protocol/VariablePolicy.h>
#include <Library/VariablePolicyLib.h>
#include <Library/VariablePolicyHelperLib.h>


/**
  DEPRECATED. THIS IS ONLY HERE AS A CONVENIENCE WHILE PORTING.
  Mark a variable that will become read-only after leaving the DXE phase of execution.
  Write request coming from SMM environment through EFI_SMM_VARIABLE_PROTOCOL is allowed.

  @param[in] This          The VARIABLE_LOCK_PROTOCOL instance.
  @param[in] VariableName  A pointer to the variable name that will be made read-only subsequently.
  @param[in] VendorGuid    A pointer to the vendor GUID that will be made read-only subsequently.

  @retval EFI_SUCCESS           The variable specified by the VariableName and the VendorGuid was marked
                                as pending to be read-only.
  @retval EFI_INVALID_PARAMETER VariableName or VendorGuid is NULL.
                                Or VariableName is an empty string.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource to hold the lock request.
**/
EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL *This,
  IN       CHAR16                       *VariableName,
  IN       EFI_GUID                     *VendorGuid
  )
{
  EFI_STATUS              Status;
  VARIABLE_POLICY_ENTRY   *NewPolicy;

  NewPolicy = NULL;
  Status = CreateBasicVariablePolicy( VendorGuid,
                                      VariableName,
                                      VARIABLE_POLICY_NO_MIN_SIZE,
                                      VARIABLE_POLICY_NO_MAX_SIZE,
                                      VARIABLE_POLICY_NO_MUST_ATTR,
                                      VARIABLE_POLICY_NO_CANT_ATTR,
                                      VARIABLE_POLICY_TYPE_LOCK_NOW,
                                      &NewPolicy );
  if (!EFI_ERROR( Status )) {
    Status = RegisterVariablePolicy( NewPolicy );
  }
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_ERROR, "%a - Failed to lock variable %s! %r\n", __FUNCTION__, VariableName, Status ));
    ASSERT_EFI_ERROR( Status );
  }
  if (NewPolicy != NULL) {
    FreePool( NewPolicy );
  }

  return Status;
}
