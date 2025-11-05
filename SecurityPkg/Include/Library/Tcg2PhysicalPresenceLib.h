/** @file
  This library is intended to be used by BDS modules.
  This library will execute TPM2 request.

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG2_PHYSICAL_PRESENCE_LIB_H_
#define _TCG2_PHYSICAL_PRESENCE_LIB_H_

#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/TcgPhysicalPresence.h>
#include <Protocol/Tcg2Protocol.h>

//
// UEFI TCG2 library definition bit of the BIOS TPM Management Flags
//
// BIT0 is reserved
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR  BIT1
// BIT2 is reserved
#define TCG2_LIB_PP_FLAG_RESET_TRACK                               BIT3
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_TURN_ON      BIT4
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_TURN_OFF     BIT5
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_EPS   BIT6
#define TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_PCRS  BIT7

//
// UEFI TCG2 library definition bit of the BIOS Information Flags
//
#define TCG2_BIOS_INFORMATION_FLAG_HIERARCHY_CONTROL_STORAGE_DISABLE      BIT8
#define TCG2_BIOS_INFORMATION_FLAG_HIERARCHY_CONTROL_ENDORSEMENT_DISABLE  BIT9

//
// UEFI TCG2 library definition bit of the BIOS Storage Management Flags
//
#define TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID   BIT16
#define TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID  BIT17
#define TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID                   BIT18

/**
  Check and execute the pending TPM request.

  The TPM request may come from OS or BIOS. This API will display request information and wait
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to
  take effect.

  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request.

  @param  PlatformAuth                   platform auth value. NULL means no platform auth change.
**/
VOID
EFIAPI
Tcg2PhysicalPresenceLibProcessRequest (
  IN      TPM2B_AUTH  *PlatformAuth  OPTIONAL
  );

/**
  Check if the pending TPM request needs user input to confirm.

  The TPM request may come from OS. This API will check if TPM request exists and need user
  input to confirmation.

  @retval    TRUE        TPM needs input to confirm user physical presence.
  @retval    FALSE       TPM doesn't need input to confirm user physical presence.

**/
BOOLEAN
EFIAPI
Tcg2PhysicalPresenceLibNeedUserConfirm (
  VOID
  );

/**
  Return TPM2 ManagementFlags set by PP interface.

  @retval    ManagementFlags    TPM2 Management Flags.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibGetManagementFlags (
  VOID
  );

/**
  The handler for TPM physical presence function:
  Return TPM Operation Response to OS Environment.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  @param[out]     MostRecentRequest Most recent operation request.
  @param[out]     Response          Response to the most recent operation request.

  @return Return Code for Return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (
  OUT UINT32  *MostRecentRequest,
  OUT UINT32  *Response
  );

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  @param[in, out]  Pointer to OperationRequest TPM physical presence operation request.
  @param[in, out]  Pointer to RequestParameter TPM physical presence operation request parameter.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
        Submit TPM Operation Request to Pre-OS Environment 2.
  **/
UINT32
Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx (
  IN OUT UINT32  *OperationRequest,
  IN OUT UINT32  *RequestParameter
  );

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      RequestParameter TPM physical presence operation request parameter.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction (
  IN UINT32  OperationRequest,
  IN UINT32  RequestParameter
  );

/**
  The handler for TPM physical presence function:
  Get User Confirmation Status for Operation.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  @param[in]      OperationRequest TPM physical presence operation request.

  @return Return Code for Get User Confirmation Status for Operation.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction (
  IN UINT32  OperationRequest
  );

#endif
