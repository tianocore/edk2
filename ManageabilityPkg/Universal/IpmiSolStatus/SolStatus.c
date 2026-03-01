/** @file
  IPMI Serial Over Lan Driver.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IpmiCommandLib.h>
#include <IndustryStandard/Ipmi.h>

#define SOL_CMD_RETRY_COUNT  10

/*++

Routine Description:

    This routine gets the SOL payload status or settings for a specific channel.

Arguments:
    Channel         - LAN channel naumber.
    ParamSel        - Configuration parameter selection.
    Data            - Information returned from BMC.
Returns:
    EFI_SUCCESS     - SOL configuration parameters are successfully read from BMC.
    Others          - SOL configuration parameters could not be read from BMC.

--*/
EFI_STATUS
GetSolStatus (
  IN UINT8      Channel,
  IN UINT8      ParamSel,
  IN OUT UINT8  *Data
  )
{
  EFI_STATUS                                      Status = EFI_SUCCESS;
  IPMI_GET_SOL_CONFIGURATION_PARAMETERS_REQUEST   GetConfigurationParametersRequest;
  IPMI_GET_SOL_CONFIGURATION_PARAMETERS_RESPONSE  GetConfigurationParametersResponse;
  UINT32                                          DataSize;
  UINT8                                           RetryCount;

  for (RetryCount = 0; RetryCount < SOL_CMD_RETRY_COUNT; RetryCount++) {
    ZeroMem (&GetConfigurationParametersRequest, sizeof (GetConfigurationParametersRequest));
    GetConfigurationParametersRequest.ChannelNumber.Bits.ChannelNumber = Channel;
    GetConfigurationParametersRequest.ParameterSelector                = ParamSel;

    ZeroMem (&GetConfigurationParametersResponse, sizeof (GetConfigurationParametersResponse));

    DataSize = sizeof (GetConfigurationParametersResponse);
    Status   = IpmiGetSolConfigurationParameters (
                 &GetConfigurationParametersRequest,
                 &GetConfigurationParametersResponse,
                 &DataSize
                 );

    if (Status == EFI_SUCCESS) {
      break;
    } else {
      gBS->Stall (100000);
    }
  }

  if (Status == EFI_SUCCESS) {
    *Data = GetConfigurationParametersResponse.ParameterData[0];
  }

  return Status;
}

/*++

Routine Description:

    This routine sets the SOL payload configuration parameters for a specific channel.

Arguments:
    Channel         - LAN channel naumber.
    ParamSel        - Configuration parameter selection.
    Data            - Configuration parameter values.
Returns:
    EFI_SUCCESS     - SOL configuration parameters are sent to BMC.
    Others          - SOL configuration parameters could not be sent to BMC.

--*/
EFI_STATUS
SetSolParams (
  IN UINT8  Channel,
  IN UINT8  ParamSel,
  IN UINT8  Data
  )
{
  EFI_STATUS                                     Status = EFI_SUCCESS;
  IPMI_SET_SOL_CONFIGURATION_PARAMETERS_REQUEST  SetConfigurationParametersRequest;
  UINT8                                          CompletionCode;
  UINT8                                          RetryCount;

  for (RetryCount = 0; RetryCount < SOL_CMD_RETRY_COUNT; RetryCount++) {
    ZeroMem (&SetConfigurationParametersRequest, sizeof (SetConfigurationParametersRequest));
    SetConfigurationParametersRequest.ChannelNumber.Bits.ChannelNumber = Channel;
    SetConfigurationParametersRequest.ParameterSelector                = ParamSel;
    SetConfigurationParametersRequest.ParameterData[0]                 = Data;

    CompletionCode = 0;

    Status = IpmiSetSolConfigurationParameters (
               &SetConfigurationParametersRequest,
               sizeof (SetConfigurationParametersRequest),
               &CompletionCode
               );

    if (Status == EFI_SUCCESS) {
      break;
    } else {
      gBS->Stall (100000);
    }
  }

  return Status;
}

/*++

  Routine Description:
    This is the standard EFI driver point. This function initializes
    the private data required for creating SOL Status Driver.

  Arguments:
    ImageHandle     - Handle for the image of this driver
    SystemTable     - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS     - Protocol successfully installed
    EFI_UNSUPPORTED - Protocol can't be installed.

--*/
EFI_STATUS
EFIAPI
SolStatusEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT8       Channel;
  BOOLEAN     SolEnabled = FALSE;

  for (Channel = 1; Channel <= PcdGet8 (PcdMaxSolChannels); Channel++) {
    Status = GetSolStatus (Channel, IPMI_SOL_CONFIGURATION_PARAMETER_SOL_ENABLE, &SolEnabled);
    if (Status == EFI_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "SOL enabling status for channel %x is %x\n", Channel, SolEnabled));
    } else {
      DEBUG ((DEBUG_ERROR, "Failed to get channel %x SOL status from BMC!, status is %x\n", Channel, Status));
    }
  }

  return Status;
}
