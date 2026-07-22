/** @file MockIpmiCommandLib.h
  Google Test mocks for IpmiCommandLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_IPMI_COMMAND_LIB_H_
#define MOCK_IPMI_COMMAND_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/IpmiCommandLib.h>
}

struct MockIpmiCommandLib {
  MOCK_INTERFACE_DECLARATION (MockIpmiCommandLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetDeviceId,
    (
     OUT IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSelfTestResult,
    (
     OUT IPMI_SELF_TEST_RESULT_RESPONSE  *SelfTestResult
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiResetWatchdogTimer,
    (
     OUT UINT8  *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetWatchdogTimer,
    (
     IN  IPMI_SET_WATCHDOG_TIMER_REQUEST  *SetWatchdogTimer,
     OUT UINT8                            *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetWatchdogTimer,
    (
     OUT IPMI_GET_WATCHDOG_TIMER_RESPONSE  *GetWatchdogTimer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetBmcGlobalEnables,
    (
     IN  IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST  *SetBmcGlobalEnables,
     OUT UINT8                                *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetBmcGlobalEnables,
    (
     OUT IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE  *GetBmcGlobalEnables
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiClearMessageFlags,
    (
     IN  IPMI_CLEAR_MESSAGE_FLAGS_REQUEST  *ClearMessageFlagsRequest,
     OUT UINT8                             *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetMessageFlags,
    (
     OUT IPMI_GET_MESSAGE_FLAGS_RESPONSE  *GetMessageFlagsResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetMessage,
    (
     OUT IPMI_GET_MESSAGE_RESPONSE  *GetMessageResponse,
     IN OUT UINT32                  *GetMessageResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSendMessage,
    (
     IN  IPMI_SEND_MESSAGE_REQUEST   *SendMessageRequest,
     IN  UINT32                      SendMessageRequestSize,
     OUT IPMI_SEND_MESSAGE_RESPONSE  *SendMessageResponse,
     IN OUT UINT32                   *SendMessageResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSystemUuid,
    (
     OUT EFI_GUID  *SystemGuid
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetChannelInfo,
    (
     IN  IPMI_GET_CHANNEL_INFO_REQUEST   *GetChannelInfoRequest,
     OUT IPMI_GET_CHANNEL_INFO_RESPONSE  *GetChannelInfoResponse,
     OUT UINT32                          *GetChannelInfoResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSystemInterfaceCapability,
    (
     IN  IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST   *InterfaceCapabilityRequest,
     OUT IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_RESPONSE  *InterfaceCapabilityResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSolActivating,
    (
     IN  IPMI_SOL_ACTIVATING_REQUEST  *SolActivatingRequest,
     OUT UINT8                        *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetSolConfigurationParameters,
    (
     IN  IPMI_SET_SOL_CONFIGURATION_PARAMETERS_REQUEST  *SetConfigurationParametersRequest,
     IN  UINT32                                         SetConfigurationParametersRequestSize,
     OUT UINT8                                          *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSolConfigurationParameters,
    (
     IN  IPMI_GET_SOL_CONFIGURATION_PARAMETERS_REQUEST   *GetConfigurationParametersRequest,
     OUT IPMI_GET_SOL_CONFIGURATION_PARAMETERS_RESPONSE  *GetConfigurationParametersResponse,
     IN OUT UINT32                                       *GetConfigurationParametersResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetLanConfigurationParameters,
    (
     IN   IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   *GetLanConfigurationParametersRequest,
     OUT  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetLanConfigurationParametersResponse,
     IN OUT UINT32                                        *GetLanConfigurationParametersSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetChassisCapabilities,
    (
     OUT IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE  *GetChassisCapabilitiesResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetChassisStatus,
    (
     OUT IPMI_GET_CHASSIS_STATUS_RESPONSE  *GetChassisStatusResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiChassisControl,
    (
     IN IPMI_CHASSIS_CONTROL_REQUEST  *ChassisControlRequest,
     OUT UINT8                        *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetPowerRestorePolicy,
    (
     IN  IPMI_SET_POWER_RESTORE_POLICY_REQUEST   *SetPowerRestireRequest,
     OUT IPMI_SET_POWER_RESTORE_POLICY_RESPONSE  *SetPowerRestireResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetSystemBootOptions,
    (
     IN  IPMI_SET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
     OUT IPMI_SET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSystemBootOptions,
    (
     IN  IPMI_GET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
     OUT IPMI_GET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetFruInventoryAreaInfo,
    (
     IN  IPMI_GET_FRU_INVENTORY_AREA_INFO_REQUEST   *GetFruInventoryAreaInfoRequest,
     OUT IPMI_GET_FRU_INVENTORY_AREA_INFO_RESPONSE  *GetFruInventoryAreaInfoResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiReadFruData,
    (
     IN  IPMI_READ_FRU_DATA_REQUEST   *ReadFruDataRequest,
     OUT IPMI_READ_FRU_DATA_RESPONSE  *ReadFruDataResponse,
     IN OUT UINT32                    *ReadFruDataResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiWriteFruData,
    (
     IN  IPMI_WRITE_FRU_DATA_REQUEST   *WriteFruDataRequest,
     IN  UINT32                        WriteFruDataRequestSize,
     OUT IPMI_WRITE_FRU_DATA_RESPONSE  *WriteFruDataResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSelInfo,
    (
     OUT IPMI_GET_SEL_INFO_RESPONSE  *GetSelInfoResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSelEntry,
    (
     IN IPMI_GET_SEL_ENTRY_REQUEST    *GetSelEntryRequest,
     OUT IPMI_GET_SEL_ENTRY_RESPONSE  *GetSelEntryResponse,
     IN OUT UINT32                    *GetSelEntryResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiAddSelEntry,
    (
     IN IPMI_ADD_SEL_ENTRY_REQUEST    *AddSelEntryRequest,
     OUT IPMI_ADD_SEL_ENTRY_RESPONSE  *AddSelEntryResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiPartialAddSelEntry,
    (
     IN IPMI_PARTIAL_ADD_SEL_ENTRY_REQUEST    *PartialAddSelEntryRequest,
     IN UINT32                                PartialAddSelEntryRequestSize,
     OUT IPMI_PARTIAL_ADD_SEL_ENTRY_RESPONSE  *PartialAddSelEntryResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiClearSel,
    (
     IN IPMI_CLEAR_SEL_REQUEST    *ClearSelRequest,
     OUT IPMI_CLEAR_SEL_RESPONSE  *ClearSelResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSelTime,
    (
     OUT IPMI_GET_SEL_TIME_RESPONSE  *GetSelTimeResponse
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetSelTime,
    (
     IN IPMI_SET_SEL_TIME_REQUEST  *SetSelTimeRequest,
     OUT UINT8                     *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSdrRepositoryInfo,
    (
     OUT IPMI_GET_SDR_REPOSITORY_INFO_RESPONSE  *GetSdrRepositoryInfoResp
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSdr,
    (
     IN  IPMI_GET_SDR_REQUEST   *GetSdrRequest,
     OUT IPMI_GET_SDR_RESPONSE  *GetSdrResponse,
     IN OUT UINT32              *GetSdrResponseSize
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiSetSensorThreshold,
    (
     IN IPMI_SENSOR_SET_SENSOR_THRESHOLD_REQUEST_DATA  *SetSensorThresholdRequestData,
     OUT UINT8                                         *CompletionCode
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    IpmiGetSensorThreshold,
    (
     IN UINT8                                            SensorNumber,
     OUT IPMI_SENSOR_GET_SENSOR_THRESHOLD_RESPONSE_DATA  *GetSensorThresholdResponse
    )
    );
};

#endif
