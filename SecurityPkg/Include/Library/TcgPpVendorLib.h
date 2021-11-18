/** @file
  This library is to support TCG Physical Presence Interface (PPI) specification
  >= 128 Vendor Specific PPI Operation.

  The Vendor Specific PPI operation may change TPM state, BIOS TPM management
  flags, and may need additional boot cycle.

  Caution: This function may receive untrusted input.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG_PP_VENDOR_LIB_H_
#define _TCG_PP_VENDOR_LIB_H_

//
// The definition of physical presence operation actions
//
#define TCG_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION  128

//
// The definition bit of the BIOS TPM Management Flags
//
#define TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION    BIT0
#define TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR        BIT1
#define TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_MAINTENANCE  BIT2
#define TCG_VENDOR_LIB_FLAG_RESET_TRACK                  BIT3

//
// The definition for TPM Operation Response to OS Environment
//
#define TCG_PP_OPERATION_RESPONSE_SUCCESS       0x0
#define TCG_PP_OPERATION_RESPONSE_USER_ABORT    0xFFFFFFF0
#define TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE  0xFFFFFFF1

//
// The return code for Submit TPM Request to Pre-OS Environment
// and Submit TPM Request to Pre-OS Environment 2
//
#define TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS                   0
#define TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED           1
#define TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE           2
#define TCG_PP_SUBMIT_REQUEST_TO_PREOS_BLOCKED_BY_BIOS_SETTINGS  3

//
// The return code for Get User Confirmation Status for Operation
//
#define TCG_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED                  0
#define TCG_PP_GET_USER_CONFIRMATION_BIOS_ONLY                        1
#define TCG_PP_GET_USER_CONFIRMATION_BLOCKED_BY_BIOS_CONFIGURATION    2
#define TCG_PP_GET_USER_CONFIRMATION_ALLOWED_AND_PPUSER_REQUIRED      3
#define TCG_PP_GET_USER_CONFIRMATION_ALLOWED_AND_PPUSER_NOT_REQUIRED  4

/**
  Check and execute the requested physical presence command.

  This API should be invoked in BIOS boot phase to process pending request.

  Caution: This function may receive untrusted input.

  If OperationRequest < 128, then ASSERT().

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in, out] ManagementFlags  BIOS TPM Management Flags.
  @param[out]     ResetRequired    If reset is required to vendor settings in effect.
                                   True, it indicates the reset is required.
                                   False, it indicates the reset is not required.

  @return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
TcgPpVendorLibExecutePendingRequest (
  IN UINT32      OperationRequest,
  IN OUT UINT32  *ManagementFlags,
  OUT BOOLEAN    *ResetRequired
  );

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
TcgPpVendorLibHasValidRequest (
  IN UINT32    OperationRequest,
  IN UINT32    ManagementFlags,
  OUT BOOLEAN  *RequestConfirmed
  );

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
TcgPpVendorLibSubmitRequestToPreOSFunction (
  IN UINT32  OperationRequest,
  IN UINT32  ManagementFlags
  );

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
TcgPpVendorLibGetUserConfirmationStatusFunction (
  IN UINT32  OperationRequest,
  IN UINT32  ManagementFlags
  );

#endif
