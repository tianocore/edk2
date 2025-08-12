/** @file
  TCG2 Standalone MM driver that updates TPM2 items in ACPI table and registers
  SMI2 callback functions for Tcg2 physical presence, ClearMemory, and
  sample for dTPM StartMethod.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable and ACPINvs data in SMM mode.
  This external input must be validated carefully to avoid security issue.

  PhysicalPresenceCallback() and MemoryClearCallback() will receive untrusted input and do some check.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>
#include <IndustryStandard/TcgPhysicalPresence.h>
#include <Guid/TpmNvsMm.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/StandaloneMmMemLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>

/**
  This function checks if the required instance is a supported TPM 2.0 instance.
  It currently supports two instances: dTPM and FFA.

  @retval TRUE  The required DTPM instance is equal to gEfiTpmDeviceInstanceTpm20DtpmGuid or gTpm2ServiceFfaGuid.
  @retval FALSE The required DTPM instance is not equal to either gEfiTpmDeviceInstanceTpm20DtpmGuid or gTpm2ServiceFfaGuid.
**/
BOOLEAN
IsTpm20InstanceSupported (
  VOID
  )
{
  VOID  *TpmGuid;

  TpmGuid = PcdGetPtr (PcdTpmInstanceGuid);
  if (TpmGuid != NULL) {
    if (CompareGuid ((EFI_GUID *)TpmGuid, &gEfiTpmDeviceInstanceTpm20DtpmGuid) ||
        CompareGuid ((EFI_GUID *)TpmGuid, &gTpm2ServiceFfaGuid))
    {
      return TRUE;
    }

    DEBUG ((DEBUG_ERROR, "Unsupported TPM2 instance configured - %g!\n", (EFI_GUID *)TpmGuid));
  } else {
    DEBUG ((DEBUG_ERROR, "NULL PcdTpmInstanceGuid set!\n"));
  }

  return FALSE;
}

/**
  Software SMI callback for TPM physical presence which is called from ACPI method.

  Caution: This function may receive untrusted input.
  Variable and ACPINvs are external input, so this function will validate
  its data structure to be valid value.

  @param[in]      DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      Context         Points to an optional handler context which was specified when the
                                  handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                  be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS             The interrupt was handled successfully.

**/
EFI_STATUS
EFIAPI
PhysicalPresenceCallback (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context,
  IN OUT VOID    *CommBuffer,
  IN OUT UINTN   *CommBufferSize
  )
{
  UINT32   MostRecentRequest;
  UINT32   Response;
  UINT32   OperationRequest;
  UINT32   RequestParameter;
  TCG_NVS  *LocalTcgNvs;

  // The handler for ARM platform should have already been copied into secure
  // memory for this case. Thus we validate the buffer a bit differently.
  if ((CommBuffer == NULL) || (CommBufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*CommBufferSize < sizeof (TCG_NVS)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MmIsBufferOutsideMmValid ((EFI_PHYSICAL_ADDRESS)(UINTN)CommBuffer, *CommBufferSize) == FALSE) {
    DEBUG ((DEBUG_ERROR, "[%a] - MM Communication buffer is outside of MM region!\n", __func__));
    return EFI_ACCESS_DENIED;
  }

  // Enough complaints, now get to work...
  LocalTcgNvs = (TCG_NVS *)CommBuffer;

  if (LocalTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS) {
    LocalTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (
                                                 &MostRecentRequest,
                                                 &Response
                                                 );
    LocalTcgNvs->PhysicalPresence.LastRequest = MostRecentRequest;
    LocalTcgNvs->PhysicalPresence.Response    = Response;
    return EFI_SUCCESS;
  } else if (  (LocalTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS)
            || (LocalTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2))
  {
    OperationRequest                         = LocalTcgNvs->PhysicalPresence.Request;
    RequestParameter                         = LocalTcgNvs->PhysicalPresence.RequestParameter;
    LocalTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx (
                                                 &OperationRequest,
                                                 &RequestParameter
                                                 );
    LocalTcgNvs->PhysicalPresence.Request          = OperationRequest;
    LocalTcgNvs->PhysicalPresence.RequestParameter = RequestParameter;
  } else if (LocalTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST) {
    LocalTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction (LocalTcgNvs->PPRequestUserConfirm);
  }

  return EFI_SUCCESS;
}

/**
  The driver's entry point.

  It install callbacks for TPM physical presence and MemoryClear, and locate
  SMM variable to be used in the callback function.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTcgStandaloneMm (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  PpSwHandle;

  if (!IsTpm20InstanceSupported ()) {
    DEBUG ((DEBUG_ERROR, "The system does not have a TPM2 DTPM/FFA instance configured, PPI not supported!\n"));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  PpSwHandle = NULL;
  Status     = gMmst->MmiHandlerRegister (PhysicalPresenceCallback, &gEfiPhysicalPresenceAcpiGuid, &PpSwHandle);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    // We have not registered the callback, so we need to return an error.
    // No need to unregister the callback since it was never registered.
    DEBUG ((DEBUG_ERROR, "[%a] Failed to register PP callback as MMI handler - %r!\n", __func__, Status));
    goto Cleanup;
  }

Cleanup:
  return Status;
}
