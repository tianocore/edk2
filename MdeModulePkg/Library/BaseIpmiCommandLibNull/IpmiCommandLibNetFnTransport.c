/** @file
  IPMI Command - NetFnTransport NULL instance library.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function activates SOL

  @param[in]      SolActivatingRequest    SOL activating request.
  @param[out]     CompletionCode          The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSolActivating (
  IN  IPMI_SOL_ACTIVATING_REQUEST  *SolActivatingRequest,
  OUT UINT8                        *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets SOL configuration parameters.

  @param[in]      SetConfigurationParametersRequest      Set SOL configuration parameters
                                                         command request.
  @param[in]      SetConfigurationParametersRequestSize  Size of set SOL configuration
                                                         parameters command request.
  @param[out]     CompletionCode                         The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetSolConfigurationParameters (
  IN  IPMI_SET_SOL_CONFIGURATION_PARAMETERS_REQUEST  *SetConfigurationParametersRequest,
  IN  UINT32                                         SetConfigurationParametersRequestSize,
  OUT UINT8                                          *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets SOL configuration parameters.

  @param[in]      GetConfigurationParametersRequest        Get SOL configuration parameters
                                                           command request.
  @param[out]     GetConfigurationParametersResponse       Get SOL configuration parameters
                                                           response.
  @param[in,out]  GetConfigurationParametersResponseSize   When input, the size of expect response.
                                                           When output, the exact size of
                                                           expect response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSolConfigurationParameters (
  IN  IPMI_GET_SOL_CONFIGURATION_PARAMETERS_REQUEST   *GetConfigurationParametersRequest,
  OUT IPMI_GET_SOL_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse,
  IN OUT UINT32                                       *GetConfigurationParametersResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets the LAN configuration parameter.

  @param[in]     GetLanConfigurationParametersRequest   Request data
  @param[out]    GetLanConfigurationParametersResponse  Response data
  @param[in,out] GetLanConfigurationParametersSize      When input, the expected size of response data.
                                                        When out, the exact size of response data.

  @retval EFI_SUCCESS          Lan configuration parameter is returned in the response.
  @retval Others               Other errors.

**/
EFI_STATUS
EFIAPI
IpmiGetLanConfigurationParameters (
  IN   IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   *GetLanConfigurationParametersRequest,
  OUT  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetLanConfigurationParametersResponse,
  IN OUT UINT32                                        *GetLanConfigurationParametersSize
  )
{
  return RETURN_UNSUPPORTED;
}
