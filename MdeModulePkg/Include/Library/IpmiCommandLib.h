/** @file
  This library abstract how to send/receive IPMI command.

Copyright (c) 2018-2021, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IPMI_COMMAND_LIB_H_
#define IPMI_COMMAND_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

//
// IPMI NetFnApp
//

/**
  This function gets the IPMI Device ID.

  @param[out] DeviceId  Get device ID response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetDeviceId (
  OUT IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
  );

/**
  This function gets the self-test result.

  @param[out] SelfTestResult  Self test command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSelfTestResult (
  OUT IPMI_SELF_TEST_RESULT_RESPONSE  *SelfTestResult
  );

/**
  This function resets watchdog timer.

  @param[out] CompletionCode  The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiResetWatchdogTimer (
  OUT UINT8  *CompletionCode
  );

/**
  This function sets watchdog timer.

  @param[in] SetWatchdogTimer  Set watchdog timer request.
  @param[out] CompletionCode   The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.


**/
EFI_STATUS
EFIAPI
IpmiSetWatchdogTimer (
  IN  IPMI_SET_WATCHDOG_TIMER_REQUEST  *SetWatchdogTimer,
  OUT UINT8                            *CompletionCode
  );

/**
  This function gets watchdog timer.

  @param[out] GetWatchdogTimer  Get watchdog timer response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetWatchdogTimer (
  OUT IPMI_GET_WATCHDOG_TIMER_RESPONSE  *GetWatchdogTimer
  );

/**
  This function sets BMC global enables.

  @param[in]  SetBmcGlobalEnables    Set BMC global enables command request.
  @param[out] CompletionCode         The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSetBmcGlobalEnables (
  IN  IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST  *SetBmcGlobalEnables,
  OUT UINT8                                *CompletionCode
  );

/**
  This function gets BMC global enables.

  @param[out]  GetBmcGlobalEnables  Get BMC global enables command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetBmcGlobalEnables (
  OUT IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE  *GetBmcGlobalEnables
  );

/**
  This function clears message flag.

  @param[in]    ClearMessageFlagsRequest  Clear message flags command request.
  @param[out]   CompletionCode            The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiClearMessageFlags (
  IN  IPMI_CLEAR_MESSAGE_FLAGS_REQUEST  *ClearMessageFlagsRequest,
  OUT UINT8                             *CompletionCode
  );

/**
  This function gets message flag.

  @param[out]    GetMessageFlagsResponse  Get message flags response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetMessageFlags (
  OUT IPMI_GET_MESSAGE_FLAGS_RESPONSE  *GetMessageFlagsResponse
  );

/**
  This function gets message.

  @param[out]     GetMessageResponse      Get message command response.
  @param[in,out]  GetMessageResponseSize  The size of get message response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetMessage (
  OUT IPMI_GET_MESSAGE_RESPONSE  *GetMessageResponse,
  IN OUT UINT32                  *GetMessageResponseSize
  );

/**
  This function sends message.

  @param[in]     SendMessageRequest        The send message command request.
  @param[in]     SendMessageRequestSize    The size of the send message command request.
  @param[out]    SendMessageResponse       The send message command response.
  @param[in,out] SendMessageResponseSize   The size of the send message command response.
                                           When input, the expected size of response.
                                           When output, the actual size of response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSendMessage (
  IN  IPMI_SEND_MESSAGE_REQUEST   *SendMessageRequest,
  IN  UINT32                      SendMessageRequestSize,
  OUT IPMI_SEND_MESSAGE_RESPONSE  *SendMessageResponse,
  IN OUT UINT32                   *SendMessageResponseSize
  );

/**
  This function gets the system UUID.

  @param[out] SystemGuid   The pointer to retrieve system UUID.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Others                 Other errors.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemUuid (
  OUT EFI_GUID  *SystemGuid
  );

/**
  This function gets the channel information.

  @param[in] GetChannelInfoRequest           The get channel information request.
  @param[in] GetChannelInfoResponse          The get channel information response.
  @param[in,out] GetChannelInfoResponseSize  When input, the expected size of response.
                                             When output, the exact size of the returned
                                             response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetChannelInfo (
  IN  IPMI_GET_CHANNEL_INFO_REQUEST   *GetChannelInfoRequest,
  OUT IPMI_GET_CHANNEL_INFO_RESPONSE  *GetChannelInfoResponse,
  OUT UINT32                          *GetChannelInfoResponseSize
  );

/**
  This function gets system interface capability

  @param[in]  InterfaceCapabilityRequest    Get system interface capability request.
  @param[out] InterfaceCapabilityResponse   The response of system interface capability.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemInterfaceCapability (
  IN  IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST   *InterfaceCapabilityRequest,
  OUT IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_RESPONSE  *InterfaceCapabilityResponse
  );

//
// IPMI NetFnTransport
//

/**
  This function activates SOL

  @param[in]      SolActivatingRequest    SOL activating request.
  @param[out]     CompletionCode          The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSolActivating (
  IN  IPMI_SOL_ACTIVATING_REQUEST  *SolActivatingRequest,
  OUT UINT8                        *CompletionCode
  );

/**
  This function sets SOL configuration parameters.

  @param[in]      SetConfigurationParametersRequest      Set SOL configuration parameters
                                                         command request.
  @param[in]      SetConfigurationParametersRequestSize  Size of the set SOL configuration
                                                         parameters command request.
  @param[out]     CompletionCode                         The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSetSolConfigurationParameters (
  IN  IPMI_SET_SOL_CONFIGURATION_PARAMETERS_REQUEST  *SetConfigurationParametersRequest,
  IN  UINT32                                         SetConfigurationParametersRequestSize,
  OUT UINT8                                          *CompletionCode
  );

/**
  This function gets SOL configuration parameters.

  @param[in]      GetConfigurationParametersRequest        Get SOL configuration parameters
                                                           command request.
  @param[out]     GetConfigurationParametersResponse       Get SOL configuration parameters
                                                           response.
  @param[in,out]  GetConfigurationParametersResponseSize   When input, the size of the expected
                                                           response.
                                                           When output, the exact size of
                                                           expect response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSolConfigurationParameters (
  IN  IPMI_GET_SOL_CONFIGURATION_PARAMETERS_REQUEST   *GetConfigurationParametersRequest,
  OUT IPMI_GET_SOL_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse,
  IN OUT UINT32                                       *GetConfigurationParametersResponseSize
  );

/**
  This function gets the LAN configuration parameter.

  @param[in]     GetLanConfigurationParametersRequest   Get LAN configuration parameters command request.
  @param[in]     GetLanConfigurationParametersResponse  The response of the get LAN configuration parameters.
  @param[in,out] GetLanConfigurationParametersSize      When input, the expected size of response data.
                                                        When out, the exact size of response data.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/

EFI_STATUS
EFIAPI
IpmiGetLanConfigurationParameters (
  IN   IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   *GetLanConfigurationParametersRequest,
  OUT  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetLanConfigurationParametersResponse,
  IN OUT UINT32                                        *GetLanConfigurationParametersSize
  );

//
// IPMI NetFnChassis
//

/**
  This function gets chassis capability.

  @param[out] GetChassisCapabilitiesResponse  Gets chassis capability command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisCapabilities (
  OUT IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE  *GetChassisCapabilitiesResponse
  );

/**
  This function gets chassis status.

  @param[out] GetChassisCapabilitiesResponse  The get chassis status command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisStatus (
  OUT IPMI_GET_CHASSIS_STATUS_RESPONSE  *GetChassisStatusResponse
  );

/**
  This function sends chassis control request.

  @param[in]  ChassisControlRequest  The chassis control request.
  @param[out] CompletionCode         The command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiChassisControl (
  IN IPMI_CHASSIS_CONTROL_REQUEST  *ChassisControlRequest,
  OUT UINT8                        *CompletionCode
  );

/**
  This function sets power restore policy.

  @param[in]  SetPowerRestireRequest    The set power restore policy control
                                        command request.
  @param[out] SetPowerRestireResponse   The response of power restore policy.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSetPowerRestorePolicy (
  IN  IPMI_SET_POWER_RESTORE_POLICY_REQUEST   *SetPowerRestireRequest,
  OUT IPMI_SET_POWER_RESTORE_POLICY_RESPONSE  *SetPowerRestireResponse
  );

//
// IPMI NetFnStorage
//

/**
  This function sets system boot option.

  @param[in]  BootOptionsRequest    Set system boot option request.
  @param[out] BootOptionsResponse   The response of set system boot
                                    option request.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSetSystemBootOptions (
  IN  IPMI_SET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_SET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  );

/**
  This function gets system boot option.

  @param[in]  BootOptionsRequest    Get system boot option request.
  @param[out] BootOptionsResponse   The response of get system boot
                                    option request.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemBootOptions (
  IN  IPMI_GET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_GET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  );

/**
  This function gets FRU inventory area info.

  @param[in]  GetFruInventoryAreaInfoRequest    Get FRU inventory area command request.
  @param[out] GetFruInventoryAreaInfoResponse   get FRU inventory area command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetFruInventoryAreaInfo (
  IN  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   *GetFruInventoryAreaInfoRequest,
  OUT IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  *GetFruInventoryAreaInfoResponse
  );

/**
  This function reads FRU data.

  @param[in]      ReadFruDataRequest       Read FRU data command request.
  @param[out]     ReadFruDataResponse      Read FRU data command response.
  @param[in,out]  ReadFruDataResponseSize  Size of the read FRU data response.
                                           When input, the expected size of response data.
                                           When out, the exact size of response data.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiReadFruData (
  IN  IPMI_READ_FRU_DATA_REQUEST   *ReadFruDataRequest,
  OUT IPMI_READ_FRU_DATA_RESPONSE  *ReadFruDataResponse,
  IN OUT UINT32                    *ReadFruDataResponseSize
  );

/**
  This function gets chassis capability.

  @param[in]    WriteFruDataRequest      Write FRU data command request.
  @param[in]    WriteFruDataRequestSize  Size of the write FRU data command request.
  @param[out]   WriteFruDataResponse     Write FRU data response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiWriteFruData (
  IN  IPMI_WRITE_FRU_DATA_REQUEST   *WriteFruDataRequest,
  IN  UINT32                        WriteFruDataRequestSize,
  OUT IPMI_WRITE_FRU_DATA_RESPONSE  *WriteFruDataResponse
  );

/**
  This function gets SEL information.

  @param[out]    GetSelInfoResponse    Get SEL information command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSelInfo (
  OUT IPMI_GET_SEL_INFO_RESPONSE  *GetSelInfoResponse
  );

/**
  This function gets SEL entry.

  @param[in]      GetSelEntryRequest       Get SEL entry command request.
  @param[out]     GetSelEntryResponse      Get SEL entry command response.
  @param[in,out]  GetSelEntryResponseSize  Size of Get SEL entry request.
                                           When input, the expected size of response data.
                                           When out, the exact size of response data.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSelEntry (
  IN IPMI_GET_SEL_ENTRY_REQUEST    *GetSelEntryRequest,
  OUT IPMI_GET_SEL_ENTRY_RESPONSE  *GetSelEntryResponse,
  IN OUT UINT32                    *GetSelEntryResponseSize
  );

/**
  This function adds SEL entry.

  @param[in]    AddSelEntryRequest       Add SEL entry command request.
  @param[out]   AddSelEntryResponse      Add SEL entry command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiAddSelEntry (
  IN IPMI_ADD_SEL_ENTRY_REQUEST    *AddSelEntryRequest,
  OUT IPMI_ADD_SEL_ENTRY_RESPONSE  *AddSelEntryResponse
  );

/**
  This function partially adds SEL entry.

  @param[in]    PartialAddSelEntryRequest      Partial add SEL entry command request.
  @param[in]    PartialAddSelEntryRequestSize  Size of partial add SEL entry command request.
  @param[out]   PartialAddSelEntryResponse     Partial add SEL entry command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiPartialAddSelEntry (
  IN IPMI_PARTIAL_ADD_SEL_ENTRY_REQUEST    *PartialAddSelEntryRequest,
  IN UINT32                                PartialAddSelEntryRequestSize,
  OUT IPMI_PARTIAL_ADD_SEL_ENTRY_RESPONSE  *PartialAddSelEntryResponse
  );

/**
  This function clears SEL entry.

  @param[in]    ClearSelRequest      Clear SEL command request.
  @param[out]   ClearSelResponse     Clear SEL command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiClearSel (
  IN IPMI_CLEAR_SEL_REQUEST    *ClearSelRequest,
  OUT IPMI_CLEAR_SEL_RESPONSE  *ClearSelResponse
  );

/**
  This function gets SEL time.

  @param[out]   GetSelTimeResponse    Get SEL time command response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSelTime (
  OUT IPMI_GET_SEL_TIME_RESPONSE  *GetSelTimeResponse
  );

/**
  This function sets SEL time.

  @param[in]    SetSelTimeRequest    Set SEL time command request.
  @param[out]   CompletionCode       Command completion code.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiSetSelTime (
  IN IPMI_SET_SEL_TIME_REQUEST  *SetSelTimeRequest,
  OUT UINT8                     *CompletionCode
  );

/**
  This function gets SDR repository information.

  @param[out]    GetSdrRepositoryInfoResp    Get SDR repository response.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.
**/
EFI_STATUS
EFIAPI
IpmiGetSdrRepositoryInfo (
  OUT IPMI_GET_SDR_REPOSITORY_INFO_RESPONSE  *GetSdrRepositoryInfoResp
  );

/**
  This function gets SDR

  @param[in]      GetSdrRequest        Get SDR resquest.
  @param[out]     GetSdrResponse       Get SDR response.
  @param[in,out]  GetSdrResponseSize   The size of get SDR response.
                                       When input, the expected size of response data.
                                       When out, the exact size of response data.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval EFI_NOT_AVAILABLE_YET  Transport interface is not ready yet.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSdr (
  IN  IPMI_GET_SDR_REQUEST   *GetSdrRequest,
  OUT IPMI_GET_SDR_RESPONSE  *GetSdrResponse,
  IN OUT UINT32              *GetSdrResponseSize
  );

#endif
