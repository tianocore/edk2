/** @file
  NULL TrEE PP Vendor library instance that does not support any vendor specific PPI.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/TrEEPpVendorLib.h>

/**
  Check and execute the requested physical presence command.

  This API should be invoked in BIOS boot phase to process pending request.
  
  Caution: This function may receive untrusted input.
  
  If OperationRequest < 128, then ASSERT().

  @param[in]      PlatformAuth     platform auth value. NULL means no platform auth change.
  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in, out] ManagementFlags  BIOS TPM Management Flags.
  @param[out]     ResetRequired    If reset is required to vendor settings in effect.
                                   True, it indicates the reset is required.
                                   False, it indicates the reset is not required.

  @return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
TrEEPpVendorLibExecutePendingRequest (
  IN TPM2B_AUTH             *PlatformAuth,  OPTIONAL
  IN UINT32                 OperationRequest,
  IN OUT UINT32             *ManagementFlags,
  OUT BOOLEAN               *ResetRequired
  )
{
  ASSERT (OperationRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION);
  return TREE_PP_OPERATION_RESPONSE_BIOS_FAILURE;
}

/**
  Check if there is a valid physical presence command request.

  This API should be invoked in BIOS boot phase to process pending request.
  
  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.
  @param[out]     RequestConfirmed If the physical presence operation command required user confirm from UI.
                                   True, it indicates the command doesn't require user confirm.
                                   False, it indicates the command need user confirm from UI.

  @retval  TRUE        Physical Presence operation command is valid.
  @retval  FALSE       Physical Presence operation command is invalid.
**/
BOOLEAN
EFIAPI
TrEEPpVendorLibHasValidRequest (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags,
  OUT BOOLEAN               *RequestConfirmed
  )
{
  ASSERT (OperationRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION);
  return FALSE;
}

/**
  The callback for TPM vendor specific physical presence which is called for
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.
  
  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
TrEEPpVendorLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags
  )
{
  ASSERT (OperationRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION);
  return TREE_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
}

/**
  The callback for TPM vendor specific physical presence which is called for
  Get User Confirmation Status for Operation.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.
  
  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      ManagementFlags  BIOS TPM Management Flags.

  @return Return Code for Get User Confirmation Status for Operation.
**/
UINT32
EFIAPI
TrEEPpVendorLibGetUserConfirmationStatusFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 ManagementFlags
  )
{
  ASSERT (OperationRequest >= TREE_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION);
  return TREE_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED;
}
