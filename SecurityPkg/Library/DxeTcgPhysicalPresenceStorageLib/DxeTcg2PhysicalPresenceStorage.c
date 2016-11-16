/** @file
  Tcg PP storage library instance that does support any storage specific PPI.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>

#include <Guid/Tcg2PhysicalPresenceData.h>
#include <Guid/TcgPhysicalPresenceStorageData.h>

#include <IndustryStandard/TcgPhysicalPresence.h>

#include <Protocol/VariableLock.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/HobLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/TcgPhysicalPresenceStorageLib.h>

#include "DxeTcgPhysicalPresenceStorageLibInternal.h"

/**
  Display the confirm text and get user confirmation.

  @param[in]      OperationRequest    TPM physical presence operation request.
  @param[in]      ManagementFlags      BIOS TPM Management Flags.


  @retval    TRUE          The user need to confirme the changes.
  @retval    FALSE         The user doesn't need to confirme the changes.
**/
BOOLEAN
Tcg2PpNeedUserConfirm (
  IN UINT8                  OperationRequest,
  IN UINT32                 ManagementFlags
  )
{
  BOOLEAN      NeedUserConfirm;

  NeedUserConfirm = FALSE;

  switch (OperationRequest) {
  case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
    if ((ManagementFlags & TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID) != 0) {
      NeedUserConfirm = TRUE;
    }
    break;

  case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
    if ((ManagementFlags & TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID) != 0) {
      NeedUserConfirm = TRUE;
    }
    break;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_TRUE:
  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_TRUE:
    NeedUserConfirm = TRUE;
    break;

  default:
    break;
  }

  return NeedUserConfirm;
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
Tcg2SubmitStorageRequest (
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  )
{
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE        PpData;

  DEBUG ((EFI_D_INFO, "[TPM Storage] SubmitRequestToPreOSFunction, Request = %x, %x\n", OperationRequest, RequestParameter));

  //
  // Get the Physical Presence storage variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM Storage] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  if ((OperationRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) &&
      (OperationRequest < TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN) ) {
    //
    // This library only support storage related actions.
    //
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_NOT_IMPLEMENTED;
  }

  if ((PpData.PPRequest != OperationRequest) ||
      (PpData.PPRequestParameter != RequestParameter)) {
    PpData.PPRequest = (UINT8)OperationRequest;
    PpData.PPRequestParameter = RequestParameter;
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status = gRT->SetVariable (
                    TCG2_PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiTcg2PhysicalPresenceGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    DataSize,
                    &PpData
                    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM2] Set PP variable failure! Status = %r\n", Status));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  return TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;
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
Tcg2NeedUserConfirm(
  VOID
  )
{
  EFI_STATUS                               Status;
  EFI_TCG2_PHYSICAL_PRESENCE               TcgPpData;
  UINTN                                    DataSize;
  EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS  PpiFlags;

  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((EFI_D_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return FALSE;
  }

  //
  // Check Tpm requests
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  DataSize = sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS);
  Status = gRT->GetVariable (
                  TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                  &gEfiTcgPhysicalPresenceStorageGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    PpiFlags.PPFlags = TCG2_BIOS_TPM_MANAGEMENT_FLAG_DEFAULT;
  }

  if ((TcgPpData.PPRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) &&
      (TcgPpData.PPRequest < TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN)) {
    //
    // This library only support storage related actions.
    //
    return FALSE;
  }

  return Tcg2PpNeedUserConfirm(TcgPpData.PPRequest, PpiFlags.PPFlags);
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
Tcg2ReturnOperationResponseToOsFunction (
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
  )
{
  EFI_STATUS                               Status;
  UINTN                                    DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE               PpData;

  DEBUG ((EFI_D_INFO, "[TPM Storage] ReturnOperationResponseToOsFunction\n"));

  //
  // Get the Physical Presence variable
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpData
                  );
  if (EFI_ERROR (Status)) {
    *MostRecentRequest = 0;
    *Response          = 0;
    DEBUG ((EFI_D_ERROR, "[TPM Storage] Get PP variable failure! Status = %r\n", Status));
    return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_FAILURE;
  }

  *MostRecentRequest = PpData.LastPPRequest;
  *Response          = PpData.PPResponse;

  return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS;
}

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
Tcg2ExecutePendingRequest (
  IN UINT8                  OperationRequest,
  IN OUT UINT32             *ManagementFlags,
  OUT BOOLEAN               *ResetRequired
  )
{
  ASSERT ((OperationRequest >= TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN) &&
          (OperationRequest < TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION));

  if (Tcg2PpNeedUserConfirm(OperationRequest, *ManagementFlags)) {
    if (!TcgPpUserConfirm (OperationRequest)) {
      return TCG_PP_OPERATION_RESPONSE_USER_ABORT;
    }
  }

  switch (OperationRequest) {
  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_TRUE:
    *ManagementFlags|= TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FUNC_FALSE:
    *ManagementFlags &= ~TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_TRUE:
    *ManagementFlags |= TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  case TCG2_PHYSICAL_PRESENCE_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FUNC_FALSE:
    *ManagementFlags &= ~TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  case TCG2_PHYSICAL_PRESENCE_ENABLE_BLOCK_SID:
    *ManagementFlags |= TCG_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  case TCG2_PHYSICAL_PRESENCE_DISABLE_BLOCK_SID:
    *ManagementFlags &= ~TCG_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID;
    return TCG_PP_OPERATION_RESPONSE_SUCCESS;

  default:
    break;
  }

  return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
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
Tcg2ProcessStorageRequest (
  VOID
  )
{
  EFI_STATUS                               Status;
  UINTN                                    DataSize;
  EFI_TCG2_PHYSICAL_PRESENCE               TcgPpData;
  EDKII_VARIABLE_LOCK_PROTOCOL             *VariableLockProtocol;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS         PpiFlags;
  EFI_TCG2_PHYSICAL_PRESENCE_FLAGS         NewPpiFlags;
  BOOLEAN                                  ResetRequired;

  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((EFI_D_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return ;
  }

  //
  // Initialize physical presence variable.
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    ZeroMem ((VOID*)&TcgPpData, sizeof (TcgPpData));
    DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
    Status   = gRT->SetVariable (
                      TCG2_PHYSICAL_PRESENCE_VARIABLE,
                      &gEfiTcg2PhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      DataSize,
                      &TcgPpData
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM Storage] Set physical presence variable failed, Status = %r\n", Status));
      return ;
    }
  }

  if ((TcgPpData.PPRequest >= TCG2_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) ||
      (TcgPpData.PPRequest < TCG2_PHYSICAL_PRESENCE_STORAGE_MANAGEMENT_BEGIN) ) {
    //
    // This library only support storage related actions.
    //
    DEBUG ((EFI_D_INFO, "[TPM Storage] Only support TCG storage related PP actions, not support PPRequest=%x\n", TcgPpData.PPRequest));
    return;
  }

  //
  // Initialize physical presence flags.
  //
  DataSize = sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS);
  Status = gRT->GetVariable (
                  TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                  &gEfiTcgPhysicalPresenceStorageGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    PpiFlags.PPFlags = TCG_BIOS_STORAGE_MANAGEMENT_FLAG_DEFAULT;
    Status   = gRT->SetVariable (
                      TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                      &gEfiTcgPhysicalPresenceStorageGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS),
                      &PpiFlags
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM Storage] Set physical presence flag failed, Status = %r\n", Status));
      return ;
    }
  }
  DEBUG ((EFI_D_INFO, "[TPM Storage] PpiFlags = %x\n", PpiFlags.PPFlags));

  //
  // This flags variable controls whether physical presence is required for TPM command.
  // It should be protected from malicious software. We set it as read-only variable here.
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLockProtocol);
  if (!EFI_ERROR (Status)) {
    Status = VariableLockProtocol->RequestToLock (
                                     VariableLockProtocol,
                                     TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                                     &gEfiTcgPhysicalPresenceStorageGuid
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[TPM Storage] Error when lock variable %s, Status = %r\n", TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }

  DEBUG ((EFI_D_INFO, "[TPM Storage] Flags=%x, PPRequest=%x (LastPPRequest=%x)\n", PpiFlags.PPFlags, TcgPpData.PPRequest, TcgPpData.LastPPRequest));

  NewPpiFlags.PPFlags = PpiFlags.PPFlags;
  ResetRequired = FALSE;
  TcgPpData.PPResponse = TCG_PP_OPERATION_RESPONSE_USER_ABORT;

  TcgPpData.PPResponse = Tcg2ExecutePendingRequest (TcgPpData.PPRequest, &NewPpiFlags.PPFlags, &ResetRequired);
  DEBUG ((EFI_D_INFO, "[TPM Storage] PPResponse = %x (LastPPRequest=%x, Flags=%x)\n", TcgPpData.PPResponse, TcgPpData.LastPPRequest, PpiFlags.PPFlags));

  if (TcgPpData.PPResponse == TCG_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Save the flags if it is updated.
  //
  if (CompareMem (&PpiFlags, &NewPpiFlags, sizeof(EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS)) != 0) {
    Status   = gRT->SetVariable (
                      TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                      &gEfiTcgPhysicalPresenceStorageGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS),
                      &NewPpiFlags
                      );
  }

  //
  // Clear request
  //
  TcgPpData.LastPPRequest = TcgPpData.PPRequest;
  TcgPpData.PPRequest = TCG2_PHYSICAL_PRESENCE_NO_ACTION;
  TcgPpData.PPRequestParameter = 0;

  //
  // Save changes
  //
  DataSize = sizeof (EFI_TCG2_PHYSICAL_PRESENCE);
  Status = gRT->SetVariable (
                  TCG2_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTcg2PhysicalPresenceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  &TcgPpData
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (!ResetRequired) {
    return;
  }

  Print (L"Rebooting system to make TPM2 settings in effect\n");
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  ASSERT (FALSE);
}

