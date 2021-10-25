/** @file
  This library is intended to be used by BDS modules.
  This library will lock TPM after executing TPM request.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG_PHYSICAL_PRESENCE_LIB_H_
#define _TCG_PHYSICAL_PRESENCE_LIB_H_

/**
  Check and execute the pending TPM request and Lock TPM.

  The TPM request may come from OS or BIOS. This API will display request information and wait
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to
  take effect. At last, it will lock TPM to prevent TPM state change by malware.

  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request. This API should also
  be invoked as early as possible as TPM is locked in this function.

**/
VOID
EFIAPI
TcgPhysicalPresenceLibProcessRequest (
  VOID
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
TcgPhysicalPresenceLibNeedUserConfirm(
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
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
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
TcgPhysicalPresenceLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest
  );

#endif
