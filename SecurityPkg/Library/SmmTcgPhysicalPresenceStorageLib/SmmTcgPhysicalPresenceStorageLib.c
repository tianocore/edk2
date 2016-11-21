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

#include <Guid/TcgPhysicalPresenceStorageData.h>
#include <IndustryStandard/TcgPhysicalPresence.h>

#include <Protocol/SmmVariable.h>



#include <Library/TcgPhysicalPresenceStorageLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>


EFI_SMM_VARIABLE_PROTOCOL  *mTcg2PpStorageSmmVariable;

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
TcgPhysicalPresenceStorageLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  )
{
  ASSERT (FALSE);

  return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS;
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
TcgPhysicalPresenceStorageLibReturnOperationResponseToOsFunction (
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
  )
{
  ASSERT (FALSE);

  return TCG_PP_RETURN_TPM_OPERATION_RESPONSE_SUCCESS;
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
TcgPhysicalPresenceStorageLibNeedUserConfirm(
  VOID
  )
{
  ASSERT (FALSE);

  return FALSE;
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
TcgPhysicalPresenceStorageLibProcessRequest (
  VOID
  )
{
  ASSERT (FALSE);
}

/**
  The handler for TPM physical presence function:
  Return TPM Operation flag variable.

  @return Return Code for Return TPM Operation flag variable.
**/
UINT32
EFIAPI
TcgPhysicalPresenceStorageLibReturnStorageFlags (
  VOID
  )
{
  UINTN                                    DataSize;
  EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS  PpiFlags;
  EFI_STATUS                               Status;

  //
  // Get the Physical Presence storage flags
  //
  DataSize = sizeof (EFI_TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS);
  Status = mTcg2PpStorageSmmVariable->SmmGetVariable (
                                 TCG_PHYSICAL_PRESENCE_STORAGE_FLAGS_VARIABLE,
                                 &gEfiTcgPhysicalPresenceStorageGuid,
                                 NULL,
                                 &DataSize,
                                 &PpiFlags
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[TPM2] Get PP storage flags failure! Status = %r\n", Status));
    PpiFlags.PPFlags = TCG_BIOS_STORAGE_MANAGEMENT_FLAG_DEFAULT;
  }

  return PpiFlags.PPFlags;
}

/**

  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceStorageLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Locate SmmVariableProtocol.
  //
  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&mTcg2PpStorageSmmVariable);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
