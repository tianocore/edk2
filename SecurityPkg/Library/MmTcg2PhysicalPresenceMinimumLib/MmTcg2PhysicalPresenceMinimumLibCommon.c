/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

  Minimized, compare to SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/MmTcg2PhysicalPresenceLibCommon.c

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Guid/Tcg2PhysicalPresenceData.h>

#include <Protocol/SmmVariable.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tcg2PpVendorLib.h>
#include <Library/MmServicesTableLib.h>

#define     PP_INF_VERSION_1_3  "1.3"

EFI_SMM_VARIABLE_PROTOCOL  *mTcg2PpSmmVariable;

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
  )
{
  EFI_STATUS                  Status;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] ReturnOperationResponseToOsFunction\n"));

  if (mTcg2PpSmmVariable == NULL) {
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }

  if ((MostRecentRequest == NULL) ||
      (Response == NULL))
  {
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = mTcg2PpSmmVariable->SmmGetVariable (
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

  This API should be invoked in OS runtime phase to interface with ACPI method.

  Caution: This function may receive untrusted input.

  @param[in, out]  Pointer to OperationRequest TPM physical presence operation request.
  @param[in, out]  Pointer to RequestParameter TPM physical presence operation request parameter.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
        Submit TPM Operation Request to Pre-OS Environment 2.
  **/
UINT32
Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx (
  IN OUT UINT32  *OperationRequest,               // TODO: who is checking these pointers?
  IN OUT UINT32  *RequestParameter
  )
{
  EFI_STATUS                  Status;
  UINT32                      ReturnCode;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] SubmitRequestToPreOSFunction, Request = %x, %x\n", *OperationRequest, *RequestParameter));
  ReturnCode = TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;

  switch (*OperationRequest) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      break;

    default:
      DEBUG ((DEBUG_WARN, "[TPM2] Unsupported PPI operation requested\n"));
      return TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
  }

  if (mTcg2PpSmmVariable == NULL) {
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  //
  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = mTcg2PpSmmVariable->SmmGetVariable (
                                   TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                   &gEfiTcg2PhysicalPresenceGuid,
                                   NULL,
                                   &DataSize,
                                   &PpData
                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    ReturnCode = TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
    goto EXIT;
  }

  if ((PpData.PPRequest != *OperationRequest) ||
      (PpData.PPRequestParameter != *RequestParameter))
  {
    PpData.PPRequest          = (UINT8)*OperationRequest;
    PpData.PPRequestParameter = *RequestParameter;
    DataSize                  = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status                    = mTcg2PpSmmVariable->SmmSetVariable (
                                                      TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                                      &gEfiTcg2PhysicalPresenceGuid,
                                                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                                      DataSize,
                                                      &PpData
                                                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[TPM2] Set PP variable failure! Status = %r\n", Status));
      ReturnCode = TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
      goto EXIT;
    }
  }

EXIT:
  //
  // Reset variable to no action on error
  //
  if (ReturnCode != TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Submit PP Request failure! Reset variable to no action. %r\n", Status));
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    ZeroMem (&PpData, DataSize);
    Status = mTcg2PpSmmVariable->SmmSetVariable (
                                   // Overwrite the variable with "No Action" to prevent DoS
                                   TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                   &gEfiTcg2PhysicalPresenceGuid,
                                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                   DataSize,
                                   &PpData
                                   );
    // Do not change variables on error, it causes Powershell to throw an exception instead of returning the ReturnCode
  }

  return ReturnCode;
}

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
  )
{
  UINT32  TempOperationRequest;
  UINT32  TempRequestParameter;

  TempOperationRequest = OperationRequest;
  TempRequestParameter = RequestParameter;

  return Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx (&TempOperationRequest, &TempRequestParameter);
}

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
  )
{
  EFI_STATUS                  Status;
  UINTN                       DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE  PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] GetUserConfirmationStatusFunction, Request = %x\n", OperationRequest));

  if (mTcg2PpSmmVariable == NULL) {
    return TCG_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED;
  }

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status   = mTcg2PpSmmVariable->SmmGetVariable (
                                   TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                   &gEfiTcg2PhysicalPresenceGuid,
                                   NULL,
                                   &DataSize,
                                   &PpData
                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_GET_USER_CONFIRMATION_BLOCKED_BY_BIOS_CONFIGURATION;
  }

  switch (OperationRequest) {
    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      return TCG_PP_GET_USER_CONFIRMATION_ALLOWED_AND_PPUSER_NOT_REQUIRED;

    default:
      return TCG_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED;
  }
}

/**
  The constructor function locates SmmVariable protocol.

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   Always return success from constructors
**/
EFI_STATUS
Tcg2PhysicalPresenceMinimumLibCommonConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  if (AsciiStrnCmp (PP_INF_VERSION_1_3, (CHAR8 *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer), sizeof (PP_INF_VERSION_1_3) - 1) != 0) {
    ASSERT (FALSE);
  }

  //
  // Locate SmmVariableProtocol.
  //
  Status = gMmst->MmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID **)&mTcg2PpSmmVariable);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
