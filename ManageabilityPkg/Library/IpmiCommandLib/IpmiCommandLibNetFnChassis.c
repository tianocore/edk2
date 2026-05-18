/** @file
  IPMI Command - NetFnChassis.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function returns information about which main chassis management functions are
  present and  what addresses are used to access those functions.

  @param [out]  GetChassisCapabilitiesResponse  Pointer to IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisCapabilities (
  OUT IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE  *GetChassisCapabilitiesResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetChassisCapabilitiesResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_GET_CAPABILITIES,
               NULL,
               0,
               (VOID *)GetChassisCapabilitiesResponse,
               &DataSize
               );
  return Status;
}

/**
  This function gets  information regarding the high-level status of the system
  chassis and main power subsystem.

  @param [out]  GetChassisStatusResponse  Pointer to IPMI_GET_CHASSIS_STATUS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisStatus (
  OUT IPMI_GET_CHASSIS_STATUS_RESPONSE  *GetChassisStatusResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetChassisStatusResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_GET_STATUS,
               NULL,
               0,
               (VOID *)GetChassisStatusResponse,
               &DataSize
               );
  return Status;
}

/**
  This function sends command to control power up, power down, and reset.

  @param [in]   ChassisControlRequest  Pointer to IPMI_CHASSIS_CONTROL_REQUEST.
  @param [out]  CompletionCode         IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiChassisControl (
  IN IPMI_CHASSIS_CONTROL_REQUEST  *ChassisControlRequest,
  OUT UINT8                        *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_CONTROL,
               (VOID *)ChassisControlRequest,
               sizeof (*ChassisControlRequest),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function is used to configure the power restore policy.

  @param [in]   ChassisControlRequest   Pointer to IPMI_SET_POWER_RESTORE_POLICY_REQUEST.
  @param [out]  ChassisControlResponse  Pointer to IPMI_SET_POWER_RESTORE_POLICY_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetPowerRestorePolicy (
  IN  IPMI_SET_POWER_RESTORE_POLICY_REQUEST   *ChassisControlRequest,
  OUT IPMI_SET_POWER_RESTORE_POLICY_RESPONSE  *ChassisControlResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*ChassisControlResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_CHASSIS,
               IPMI_CHASSIS_SET_POWER_RESTORE_POLICY,
               (VOID *)ChassisControlRequest,
               sizeof (*ChassisControlRequest),
               (VOID *)ChassisControlResponse,
               &DataSize
               );
  return Status;
}

/**
  This function is used to set parameters that direct the system boot
  following a system power up or reset.

  @param [in]   BootOptionsRequest   Pointer to IPMI_SET_BOOT_OPTIONS_REQUEST.
  @param [out]  BootOptionsResponse  Pointer to IPMI_SET_BOOT_OPTIONS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetSystemBootOptions (
  IN  IPMI_SET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_SET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  EFI_STATUS  Status;
  UINT32      RequestDataSize;
  UINT32      ResponseDataSize;

  ResponseDataSize = sizeof (*BootOptionsResponse);
  RequestDataSize  = sizeof (*BootOptionsRequest);

  switch (BootOptionsRequest->ParameterValid.Bits.ParameterSelector) {
    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SET_IN_PROGRESS:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_0);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SELECTOR:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_1);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SCAN:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_2);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_BMC_BOOT_FLAG:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_3);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_INFO:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_6);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX:
      RequestDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7);
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IpmiSubmitCommand (
             IPMI_NETFN_CHASSIS,
             IPMI_CHASSIS_SET_SYSTEM_BOOT_OPTIONS,
             (VOID *)BootOptionsRequest,
             RequestDataSize,
             (VOID *)BootOptionsResponse,
             &ResponseDataSize
             );
  return Status;
}

/**
  This function is used to retrieve the boot options set by the
  Set System Boot Options command.

  @param [in]   BootOptionsRequest   Pointer to IPMI_GET_BOOT_OPTIONS_REQUEST.
  @param [out]  BootOptionsResponse  Pointer to IPMI_GET_BOOT_OPTIONS_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemBootOptions (
  IN  IPMI_GET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_GET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseDataSize;

  ResponseDataSize = sizeof (*BootOptionsResponse);

  switch (BootOptionsRequest->ParameterSelector.Bits.ParameterSelector) {
    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SET_IN_PROGRESS:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_0);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SELECTOR:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_1);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_SERVICE_PARTITION_SCAN:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_2);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_SELECTOR_BMC_BOOT_FLAG:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_3);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INFO_ACK:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_4);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_INFO:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_6);
      break;

    case IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX:
      ResponseDataSize += sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7);
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  Status = IpmiSubmitCommand (
             IPMI_NETFN_CHASSIS,
             IPMI_CHASSIS_GET_SYSTEM_BOOT_OPTIONS,
             (VOID *)BootOptionsRequest,
             sizeof (*BootOptionsRequest),
             (VOID *)BootOptionsResponse,
             &ResponseDataSize
             );
  return Status;
}

/**
  Write data to the IPMI boot initiator mailbox helper function.
  This function calls the IpmiSetSystemBootOptions function with
  parameter 7 to write a maximum of 16 bytes to a selector block
  in the boot initiator mailbox.

  @param[in] SetSelector    Set selector to write to
  @param[in] Data           Data to write to the mailbox
  @param[in] Size           Size of Data buffer in bytes

  @retval EFI_SUCCESS   On successfull IPMI transaction
  @retval EFI_INVALID   Data or Size is not valid
  @retval Other         On failing IPMI transaction
**/
EFI_STATUS
EFIAPI
IpmiWriteBootInitiatorMailbox (
  IN UINT8  SetSelector,
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINT8                           *SetBootOptionsBuffer;
  IPMI_SET_BOOT_OPTIONS_REQUEST   *SetBootOptionsRequest;
  IPMI_SET_BOOT_OPTIONS_RESPONSE  SetBootOptionsResponse;
  EFI_STATUS                      Status;

  if ((Data == NULL) || (Size == 0) || (Size > 16)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));

  SetBootOptionsBuffer = (UINT8 *)AllocateZeroPool (sizeof (*SetBootOptionsRequest) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7));
  if (SetBootOptionsBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetBootOptionsBuffer[sizeof (*SetBootOptionsRequest)] = SetSelector;
  CopyMem (&SetBootOptionsBuffer[sizeof (*SetBootOptionsRequest) + 1], Data, Size);

  SetBootOptionsRequest                                           = (IPMI_SET_BOOT_OPTIONS_REQUEST *)&SetBootOptionsBuffer[0];
  SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector    = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX;
  SetBootOptionsRequest->ParameterValid.Bits.MarkParameterInvalid = 0; // mark as valid parameter

  Status = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);
  DEBUG ((DEBUG_INFO, "IpmiBootInitiator mailbox WRITE completion code 0x%X\n", SetBootOptionsResponse.CompletionCode));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Writing IPMI Boot Initiator mailbox unsuccessfull, Status - %r\n", Status));
  }

  FreePool (SetBootOptionsBuffer);
  return Status;
}

/**
  Read data from the IPMI boot initiator mailbox helper function.
  This function calls the IpmiGetSystemBootOptions function with
  parameter 7 to read a 16 byte selector block in the boot initiator
  mailbox. ReadData is a 16 byte array and is required to be freed
  by the caller.

  @param[in]  SetSelector    Set selector to read from
  @param[out] ReadData       Data read from the Boot Initiator Mailbox

  @retval EFI_SUCCESS   On successfull IPMI transaction
  @retval EFI_INVALID   ReadData is NULL
  @retval Other         On failing IPMI transaction
**/
EFI_STATUS
EFIAPI
IpmiReadBootInitiatorMailbox (
  IN UINT8   SetSelector,
  OUT UINT8  **ReadData
  )
{
  UINT8                                   *GetBootOptionsBuffer;
  IPMI_GET_BOOT_OPTIONS_REQUEST           BootOptionsRequest;
  IPMI_GET_BOOT_OPTIONS_RESPONSE          *BootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7  *BootOptionsParameterData;
  UINT8                                   *BlockData;
  EFI_STATUS                              Status;
  UINT8                                   Count;

  if (ReadData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));
  GetBootOptionsBuffer = AllocateZeroPool (sizeof (BootOptionsResponse) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7));
  if (GetBootOptionsBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlockData = AllocateZeroPool (sizeof (BootOptionsParameterData->BlockData));
  if (BlockData == NULL) {
    FreePool (GetBootOptionsBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  // setup parameter data
  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_INITIATOR_MAILBOX;
  BootOptionsRequest.SetSelector                              = SetSelector;
  BootOptionsResponse                                         = (IPMI_GET_BOOT_OPTIONS_RESPONSE *)&GetBootOptionsBuffer[0];
  Status                                                      = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);

  BootOptionsParameterData = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_7 *)BootOptionsResponse->ParameterData;

  // A parameter is invalid when a value of 1 is returned
  if (EFI_ERROR (Status) || BootOptionsResponse->ParameterValid.Bits.ParameterValid) {
    DEBUG ((
      DEBUG_INFO,
      "Reading IPMI Boot Initiator mailbox unsuccessful, Status - %r Parameter Valid = %d\n",
      Status,
      BootOptionsResponse->ParameterValid.Bits.ParameterValid
      ));
    FreePool (BlockData);
    FreePool (GetBootOptionsBuffer);
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "IPMI Mailbox Read Data: "));
  for (Count = 0; Count < 16; Count++) {
    DEBUG ((DEBUG_VERBOSE, "%x ", BootOptionsParameterData->BlockData[Count]));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));

  // Copy data over to buffer and return this
  CopyMem (BlockData, BootOptionsParameterData->BlockData, sizeof (BootOptionsParameterData->BlockData));
  *ReadData = BlockData;
  FreePool (GetBootOptionsBuffer);
  return EFI_SUCCESS;
}
