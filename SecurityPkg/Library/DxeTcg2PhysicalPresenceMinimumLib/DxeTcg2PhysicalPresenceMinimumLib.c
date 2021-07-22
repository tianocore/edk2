/** @file
  Execute pending TPM2 requests from OS or BIOS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable. This external
  input must be validated carefully to avoid security issue.

  Tpm2ExecutePendingTpmRequest() will receive untrusted input and do validation.

  Simplifying, removing all optional operations, auto-approving all Clear requests,
  diff versus SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.c
  Note that Physical Presence flags are no longer needed, and are removed

Copyright (c) 2013 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/Tcg2Protocol.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Guid/Tcg2PhysicalPresenceData.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>

/**
  Send ClearControl and Clear command to TPM.

  @param[in]  PlatformAuth      platform auth value. NULL means no platform auth change.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_TIMEOUT           The register can't run into the expected status in time.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.

**/
EFI_STATUS
EFIAPI
Tpm2CommandClear (
  IN TPM2B_AUTH  *PlatformAuth  OPTIONAL
  )
{
  EFI_STATUS         Status;
  TPMS_AUTH_COMMAND  *AuthSession;
  TPMS_AUTH_COMMAND  LocalAuthSession;

  if (PlatformAuth == NULL) {
    AuthSession = NULL;
  } else {
    AuthSession = &LocalAuthSession;
    ZeroMem (&LocalAuthSession, sizeof (LocalAuthSession));
    LocalAuthSession.sessionHandle = TPM_RS_PW;
    LocalAuthSession.hmac.size     = PlatformAuth->size;
    CopyMem (LocalAuthSession.hmac.buffer, PlatformAuth->buffer, PlatformAuth->size);
  }

  DEBUG ((DEBUG_INFO, "Tpm2ClearControl ... \n"));
  Status = Tpm2ClearControl (TPM_RH_PLATFORM, AuthSession, NO);
  DEBUG ((DEBUG_INFO, "Tpm2ClearControl - %r\n", Status));
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DEBUG ((DEBUG_INFO, "Tpm2Clear ... \n"));
  Status = Tpm2Clear (TPM_RH_PLATFORM, AuthSession);
  DEBUG ((DEBUG_INFO, "Tpm2Clear - %r\n", Status));

Done:
  ZeroMem (&LocalAuthSession.hmac, sizeof (LocalAuthSession.hmac));
  return Status;
}

/**
  Execute physical presence operation requested by the OS.

  @param[in]      PlatformAuth        platform auth value. NULL means no platform auth change.
  @param[in]      CommandCode         Physical presence operation value.
  @param[in]      CommandParameter    Physical presence operation parameter.

  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Unknown physical presence operation.
  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE   Error occurred during sending command to TPM or
                                                   receiving response from TPM.
  @retval Others                                   Return code from the TPM device after command execution.
**/
UINT32
Tcg2ExecutePhysicalPresence (
  IN      TPM2B_AUTH *PlatformAuth, OPTIONAL
  IN      UINT32                           CommandCode,
  IN      UINT32                           CommandParameter
  )
{
  EFI_STATUS  Status;

  switch (CommandCode) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      Status = Tpm2CommandClear (PlatformAuth);
      if (EFI_ERROR (Status)) {
        return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
      } else {
        return TCG_PP_OPERATION_RESPONSE_SUCCESS;
      }

    default:
      return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
  }
}

/**
  Check if there is a valid physical presence command request. Also updates parameter value
  to whether the requested physical presence command already confirmed by user

   @param[in]  TcgPpData                 EFI Tcg2 Physical Presence request data.
   @param[out] RequestConfirmed          If the physical presence operation command required user confirm from UI.
                                           True, it indicates the command doesn't require user confirm, or already confirmed
                                                 in last boot cycle by user.
                                           False, it indicates the command need user confirm from UI.

   @retval  TRUE        Physical Presence operation command is valid.
   @retval  FALSE       Physical Presence operation command is invalid.

**/
BOOLEAN
Tcg2HaveValidTpmRequest  (
  IN      EFI_TCG2_PHYSICAL_PRESENCE  *TcgPpData,
  OUT     BOOLEAN                     *RequestConfirmed
  )
{
  *RequestConfirmed = FALSE;

  switch (TcgPpData->PPRequest) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
      *RequestConfirmed = TRUE;
      return TRUE;

    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      *RequestConfirmed = TRUE;
      break;

    default:
      //
      // Wrong Physical Presence command
      //
      return FALSE;
  }

  //
  // Physical Presence command is correct
  //
  return TRUE;
}

/**
  Check and execute the requested physical presence command.

  Caution: This function may receive untrusted input.
  TcgPpData variable is external input, so this function will validate
  its data structure to be valid value.

  @param[in]      PlatformAuth      platform auth value. NULL means no platform auth change.
  @param[in, out] TcgPpData         Pointer to the physical presence NV variable.
**/
VOID
Tcg2ExecutePendingTpmRequest (
  IN      TPM2B_AUTH *PlatformAuth, OPTIONAL
  IN OUT  EFI_TCG2_PHYSICAL_PRESENCE       *TcgPpData
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  BOOLEAN     RequestConfirmed;

  if (TcgPpData->PPRequest == TCG2_PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return;
  }

  if (!Tcg2HaveValidTpmRequest (TcgPpData, &RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    if (TcgPpData->PPRequest <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
      TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_SUCCESS;  // TODO: why is this not BIOS_FAILURE? Speculation is that it causes WHQL tests to fail (test bug?)
    } else {
      TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
    }

    TcgPpData->LastPPRequest      = TcgPpData->PPRequest;
    TcgPpData->PPRequest          = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
    TcgPpData->PPRequestParameter = 0;

    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      DataSize,
                      TcgPpData
                      );
    return;
  }

  if (!RequestConfirmed) {
    //
    // Print confirm text and wait for approval.
    //
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  }

  //
  // Execute requested physical presence command
  //
  TcgPpData->PPResponse = TCG_PP_OPERATION_RESPONSE_USER_ABORT;
  if (RequestConfirmed) {
    TcgPpData->PPResponse = Tcg2ExecutePhysicalPresence (
                              PlatformAuth,
                              TcgPpData->PPRequest,
                              TcgPpData->PPRequestParameter
                              );
  }

  //
  // Clear request
  //
  TcgPpData->LastPPRequest      = TcgPpData->PPRequest;
  TcgPpData->PPRequest          = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
  TcgPpData->PPRequestParameter = 0;

  //
  // Save changes
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = gRT->SetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    TcgPpData
                    );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (TcgPpData->PPResponse == TCG_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (TcgPpData->LastPPRequest) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      break;

    default:
      return;
  }

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  ASSERT (FALSE);
}

/**
  Check and execute the pending TPM request.

  The TPM request may come from OS or BIOS. This API will display request information and wait
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to
  take effect.

  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request.

  @param[in]  PlatformAuth                   platform auth value. NULL means no platform auth change.
**/
VOID
EFIAPI
Tcg2PhysicalPresenceLibProcessRequest (
  IN      TPM2B_AUTH  *PlatformAuth  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  TcgPpData;
  EFI_BOOT_MODE               BootMode;

  DEBUG ((DEBUG_INFO, "[TPM2] Tcg2PhysicalPresenceLibProcessRequest Entry...\n"));

  //
  // Check S4 resume
  //
  BootMode = GetBootModeHob ();
  if ((BootMode == BOOT_ON_S4_RESUME) ||
      (BootMode == BOOT_ON_FLASH_UPDATE))
  {
    DEBUG ((DEBUG_INFO, "S4 Resume or Flash Update, Skip TPM PP process!\n"));
    return;
  }

  //
  // Initialize physical presence variable.
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = gRT->GetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    NULL,
                    &DataSize,
                    &TcgPpData
                    );
  if (EFI_ERROR (Status)) {
    ZeroMem ((VOID *)&TcgPpData, sizeof (TcgPpData));
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      DataSize,
                      &TcgPpData
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[TPM2] Set physical presence variable failed, Status = %r\n", Status));
      return;
    }
  }

  DEBUG ((DEBUG_INFO, "[TPM2] PPRequest=%x (LastPPRequest=%x)\n", TcgPpData.PPRequest, TcgPpData.LastPPRequest));

  //
  // Execute pending TPM request.
  //
  Tcg2ExecutePendingTpmRequest (PlatformAuth, &TcgPpData);
  DEBUG ((DEBUG_INFO, "[TPM2] PPResponse = %x (LastPPRequest=%x)\n", TcgPpData.PPResponse, TcgPpData.LastPPRequest));
}

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
  )
{
  return FALSE;
}

/**
  The handler for TPM physical presence function:
  Return TPM Operation Response to OS Environment.

  @param[out]     MostRecentRequest Most recent operation request.
  @param[out]     Response          Response to the most recent operation request.

  @return Return Code for Return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (
  OUT UINT32  *MostRecentRequest,
  OUT UINT32  *Response
  )
{
  EFI_STATUS                  Status;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] ReturnOperationResponseToOsFunction\n"));

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = gRT->GetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    NULL,
                    &DataSize,
                    &PpData
                    );
  if (EFI_ERROR (Status)) {
    *MostRecentRequest = 0;
    *Response          = 0;
    DEBUG ((DEBUG_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }

  *MostRecentRequest = PpData.LastPPRequest;
  *Response          = PpData.PPResponse;

  return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS;
}

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

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
  )
{
  EFI_STATUS                  Status;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] SubmitRequestToPreOSFunction, Request = %x, %x\n", OperationRequest, RequestParameter));

  switch (OperationRequest) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      break;

    default:
      return TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
  }

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = gRT->GetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    NULL,
                    &DataSize,
                    &PpData
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  if ((PpData.PPRequest != OperationRequest) ||
      (PpData.PPRequestParameter != RequestParameter))
  {
    PpData.PPRequest          = (UINT8)OperationRequest;
    PpData.PPRequestParameter = RequestParameter;
    DataSize                  = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status                    = gRT->SetVariable (
                                       TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                       &gEfiTcg2PhysicalPresenceGuid,
                                       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                       DataSize,
                                       &PpData
                                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[TPM2] Set PP variable failure! Status = %r\n", Status));
      return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
    }
  }

  return TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;
}
