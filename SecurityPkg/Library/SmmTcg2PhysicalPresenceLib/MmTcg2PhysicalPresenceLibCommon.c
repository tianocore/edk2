/** @file
  Handle TPM 2.0 physical presence requests from OS.

  This library will handle TPM 2.0 physical presence request from OS.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunction() and Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction()
  will receive untrusted input and do validation.

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.<BR>
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

#define     PP_INF_VERSION_1_2    "1.2"

EFI_SMM_VARIABLE_PROTOCOL  *mTcg2PpSmmVariable;
BOOLEAN                    mIsTcg2PPVerLowerThan_1_3 = FALSE;
UINT32                     mTcg2PhysicalPresenceFlags;

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
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;

  DEBUG ((DEBUG_INFO, "[TPM2] ReturnOperationResponseToOsFunction\n"));

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = mTcg2PpSmmVariable->SmmGetVariable (
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
  IN OUT UINT32               *OperationRequest,
  IN OUT UINT32               *RequestParameter
  )
{
  EFI_STATUS                        Status;
  UINT32                            ReturnCode;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  Flags;

  DEBUG ((DEBUG_INFO, "[TPM2] SubmitRequestToPreOSFunction, Request = %x, %x\n", *OperationRequest, *RequestParameter));
  ReturnCode = TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = mTcg2PpSmmVariable->SmmGetVariable (
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

  if ((*OperationRequest > TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) &&
      (*OperationRequest < TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN) ) {
    ReturnCode = TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
    goto EXIT;
  }

  if ((PpData.PPRequest != *OperationRequest) ||
      (PpData.PPRequestParameter != *RequestParameter)) {
    PpData.PPRequest = (UINT8)*OperationRequest;
    PpData.PPRequestParameter = *RequestParameter;
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status = mTcg2PpSmmVariable->SmmSetVariable (
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

  if (*OperationRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS);
    Status = mTcg2PpSmmVariable->SmmGetVariable (
                                   TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                                   &gEfiTcg2PhysicalPresenceGuid,
                                   NULL,
                                   &DataSize,
                                   &Flags
                                   );
    if (EFI_ERROR (Status)) {
      Flags.PPFlags = mTcg2PhysicalPresenceFlags;
    }
    ReturnCode = Tcg2PpVendorLibSubmitRequestToPreOSFunction (*OperationRequest, Flags.PPFlags, *RequestParameter);
  }

EXIT:
  //
  // Sync PPRQ/PPRM from PP Variable if PP submission fails
  //
  if (ReturnCode != TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Submit PP Request failure! Sync PPRQ/PPRM with PP variable.\n", Status));
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    ZeroMem(&PpData, DataSize);
    Status = mTcg2PpSmmVariable->SmmGetVariable (
                                   TCG2_PHYSICAL_PRESENCE_VARIABLE,
                                   &gEfiTcg2PhysicalPresenceGuid,
                                   NULL,
                                   &DataSize,
                                   &PpData
                                   );
    *OperationRequest = (UINT32)PpData.PPRequest;
    *RequestParameter = PpData.PPRequestParameter;
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
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  )
{
  UINT32                 TempOperationRequest;
  UINT32                 TempRequestParameter;

  TempOperationRequest = OperationRequest;
  TempRequestParameter = RequestParameter;

  return Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx(&TempOperationRequest, &TempRequestParameter);
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
  IN UINT32                 OperationRequest
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS  Flags;
  BOOLEAN                           RequestConfirmed;

  DEBUG ((DEBUG_INFO, "[TPM2] GetUserConfirmationStatusFunction, Request = %x\n", OperationRequest));

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = mTcg2PpSmmVariable->SmmGetVariable (
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
  //
  // Get the Physical Presence flags
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE_FLAGS);
  Status = mTcg2PpSmmVariable->SmmGetVariable (
                                 TCG2_PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                                 &gEfiTcg2PhysicalPresenceGuid,
                                 NULL,
                                 &DataSize,
                                 &Flags
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[TPM2] Get PP flags failure! Status = %r\n", Status));
    return TCG_PP_GET_USER_CONFIRMATION_BLOCKED_BY_BIOS_CONFIGURATION;
  }

  RequestConfirmed = FALSE;

  switch (OperationRequest) {
    case TCG2_PHYSICAL_PRESENCE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_2:
    case TCG2_PHYSICAL_PRESENCE_ENABLE_CLEAR_3:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CLEAR) == 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_NO_ACTION:
    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_TRUE:
      RequestConfirmed = TRUE;
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_CLEAR_FALSE:
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PCR_BANKS:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_PCRS) == 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_CHANGE_EPS:
      if ((Flags.PPFlags & TCG2_BIOS_TPM_MANAGEMENT_FLAG_PP_REQUIRED_FOR_CHANGE_EPS) == 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_LOG_ALL_DIGESTS:
      RequestConfirmed = TRUE;
      break;

    case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
      if ((Flags.PPFlags & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID) == 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
      if ((Flags.PPFlags & TCG2_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID) == 0) {
        RequestConfirmed = TRUE;
      }
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_TRUE:
    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_TRUE:
      RequestConfirmed = TRUE;
      break;

    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_FALSE:
    case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_FALSE:
      break;

    default:
      if (!mIsTcg2PPVerLowerThan_1_3) {
        if (OperationRequest < TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
          //
          // TCG2 PP1.3 spec defined operations that are reserved or un-implemented
          //
          return TCG_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED;
        }
      } else {
       //
       // TCG PP lower than 1.3. (1.0, 1.1, 1.2)
       //
       if (OperationRequest <= TCG2_PHYSICAL_PRESENCE_NO_ACTION_MAX) {
         RequestConfirmed = TRUE;
       } else if (OperationRequest < TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
         return TCG_PP_GET_USER_CONFIRMATION_NOT_IMPLEMENTED;
       }
      }
      break;
  }

  if (OperationRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
    return Tcg2PpVendorLibGetUserConfirmationStatusFunction (OperationRequest, Flags.PPFlags);
  }

  if (RequestConfirmed) {
    return TCG_PP_GET_USER_CONFIRMATION_ALLOWED_AND_PPUSER_NOT_REQUIRED;
  } else {
    return TCG_PP_GET_USER_CONFIRMATION_ALLOWED_AND_PPUSER_REQUIRED;
  }
}

/**
  The constructor function locates SmmVariable protocol.

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @retval EFI_SUCCESS   The constructor successfully added string package.
  @retval Other value   The constructor can't add string package.
**/
EFI_STATUS
Tcg2PhysicalPresenceLibCommonConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  if (AsciiStrnCmp(PP_INF_VERSION_1_2, (CHAR8 *)PcdGetPtr(PcdTcgPhysicalPresenceInterfaceVer), sizeof(PP_INF_VERSION_1_2) - 1) >= 0) {
    mIsTcg2PPVerLowerThan_1_3 = TRUE;
  }

  //
  // Locate SmmVariableProtocol.
  //
  Status = gMmst->MmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&mTcg2PpSmmVariable);
  ASSERT_EFI_ERROR (Status);

  mTcg2PhysicalPresenceFlags = PcdGet32(PcdTcg2PhysicalPresenceFlags);

  return EFI_SUCCESS;
}
