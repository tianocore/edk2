/** @file
  IPMI Command - NetFnApp.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>

#include <IndustryStandard/Ipmi.h>

/**
  This function is used to retrieve device ID.

  @param [out]  DeviceId  The pointer to receive IPMI_GET_DEVICE_ID_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetDeviceId (
  OUT IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*DeviceId);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_GET_DEVICE_ID,
               NULL,
               0,
               (VOID *)DeviceId,
               &DataSize
               );
  return Status;
}

/**
  This function returns device self test results

  @param [out]  SelfTestResult  The pointer to receive IPMI_SELF_TEST_RESULT_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSelfTestResult (
  OUT IPMI_SELF_TEST_RESULT_RESPONSE  *SelfTestResult
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*SelfTestResult);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_GET_SELFTEST_RESULTS,
               NULL,
               0,
               (VOID *)SelfTestResult,
               &DataSize
               );
  return Status;
}

/**
  This function is used for starting and restarting the Watchdog
  Timer from the initial countdown value that was specified in
  the Set Watchdog Timer command the watchdog timer

  @param [out]  CompletionCode  IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiResetWatchdogTimer (
  OUT UINT8  *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_RESET_WATCHDOG_TIMER,
               NULL,
               0,
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function  is used for initializing and configuring
  the watchdog timer.

  @param [in]   SetWatchdogTimer  Pointer to receive IPMI_SET_WATCHDOG_TIMER_REQUEST.
  @param [out]  CompletionCode    IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetWatchdogTimer (
  IN  IPMI_SET_WATCHDOG_TIMER_REQUEST  *SetWatchdogTimer,
  OUT UINT8                            *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_SET_WATCHDOG_TIMER,
               (VOID *)SetWatchdogTimer,
               sizeof (*SetWatchdogTimer),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function retrieves the current settings and present
  countdown of the watchdog timer.

  @param [out]  GetWatchdogTimer  Pointer to receive IPMI_GET_WATCHDOG_TIMER_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetWatchdogTimer (
  OUT IPMI_GET_WATCHDOG_TIMER_RESPONSE  *GetWatchdogTimer
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetWatchdogTimer);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_GET_WATCHDOG_TIMER,
               NULL,
               0,
               (VOID *)GetWatchdogTimer,
               &DataSize
               );
  return Status;
}

/**
  This function enables message reception into Message Buffers,
  and any interrupt associated with that buffer getting full.

  @param [in]   SetBmcGlobalEnables  Pointer receive to IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST.
  @param [out]  CompletionCode      IPMI completetion code refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSetBmcGlobalEnables (
  IN  IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST  *SetBmcGlobalEnables,
  OUT UINT8                                *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_SET_BMC_GLOBAL_ENABLES,
               (VOID *)SetBmcGlobalEnables,
               sizeof (*SetBmcGlobalEnables),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function retrieves the present setting of the Global Enables

  @param [out]  GetBmcGlobalEnables  Pointer to receive IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetBmcGlobalEnables (
  OUT IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE  *GetBmcGlobalEnables
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetBmcGlobalEnables);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_GET_BMC_GLOBAL_ENABLES,
               NULL,
               0,
               (VOID *)GetBmcGlobalEnables,
               &DataSize
               );
  return Status;
}

/**
  This function is used to flush unread data from the Receive
  Message Queue or Event Message Buffer

  @param [in]   ClearMessageFlagsRequest IPMI_CLEAR_MESSAGE_FLAGS_REQUEST
  @param [out]  CompletionCode           IPMI completetion code, refer to Ipmi.h.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiClearMessageFlags (
  IN  IPMI_CLEAR_MESSAGE_FLAGS_REQUEST  *ClearMessageFlagsRequest,
  OUT UINT8                             *CompletionCode
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*CompletionCode);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_CLEAR_MESSAGE_FLAGS,
               (VOID *)ClearMessageFlagsRequest,
               sizeof (*ClearMessageFlagsRequest),
               (VOID *)CompletionCode,
               &DataSize
               );
  return Status;
}

/**
  This function is used to retrieve the present message available states.

  @param  [out]  GetMessageFlagsResponse  Pointer to receive IPMI_GET_MESSAGE_FLAGS_RESPONSE

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetMessageFlags (
  OUT IPMI_GET_MESSAGE_FLAGS_RESPONSE  *GetMessageFlagsResponse
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  DataSize = sizeof (*GetMessageFlagsResponse);
  Status   = IpmiSubmitCommand (
               IPMI_NETFN_APP,
               IPMI_APP_GET_MESSAGE_FLAGS,
               NULL,
               0,
               (VOID *)GetMessageFlagsResponse,
               &DataSize
               );
  return Status;
}

/**
  This function is used to get data from the Receive Message Queue.

  @param [out]      GetMessageResponse      Pointer to receive IPMI_GET_MESSAGE_RESPONSE.
  @param [in, out]  GetMessageResponseSize  When in, which is the expected size of
                                            response. When out, which is the actual
                                            size returned.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetMessage (
  OUT IPMI_GET_MESSAGE_RESPONSE  *GetMessageResponse,
  IN OUT UINT32                  *GetMessageResponseSize
  )
{
  EFI_STATUS  Status;

  Status = IpmiSubmitCommand (
             IPMI_NETFN_APP,
             IPMI_APP_GET_MESSAGE,
             NULL,
             0,
             (VOID *)GetMessageResponse,
             GetMessageResponseSize
             );
  return Status;
}

/**
  This function is used for bridging IPMI messages between channels,
  and between the system management software (SMS) and a given channel

  @param [in]   SendMessageRequest       Pointer to IPMI_SEND_MESSAGE_REQUEST.
  @param [in]   SendMessageRequestSize   Size of entire SendMessageRequestSize.
  @param [out]  SendMessageResponse      Pointer to receive IPMI_SEND_MESSAGE_RESPONSE.
  @param [in]   SendMessageResponseSize  When in, which is the expected size of
                                         response. When out, which is the actual
                                         size returned.

  @retval EFI_STATUS   See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiSendMessage (
  IN  IPMI_SEND_MESSAGE_REQUEST   *SendMessageRequest,
  IN  UINT32                      SendMessageRequestSize,
  OUT IPMI_SEND_MESSAGE_RESPONSE  *SendMessageResponse,
  IN OUT UINT32                   *SendMessageResponseSize
  )
{
  EFI_STATUS  Status;

  Status = IpmiSubmitCommand (
             IPMI_NETFN_APP,
             IPMI_APP_SEND_MESSAGE,
             (VOID *)SendMessageRequest,
             SendMessageRequestSize,
             (VOID *)SendMessageResponse,
             SendMessageResponseSize
             );
  return Status;
}

/**
  This function gets the system UUID.

  @param[out] SystemGuid   The pointer to retrieve system UUID.

  @retval EFI_SUCCESS               UUID is returned.
  @retval EFI_INVALID_PARAMETER     SystemGuid is a NULL pointer.
  @retval Others                    See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemUuid (
  OUT EFI_GUID  *SystemGuid
  )
{
  EFI_STATUS                     Status;
  UINT32                         RequestSize;
  UINT32                         ResponseSize;
  IPMI_GET_SYSTEM_UUID_RESPONSE  GetSystemUuidResponse;

  if (SystemGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RequestSize  = 0;
  ResponseSize = sizeof (IPMI_GET_SYSTEM_UUID_RESPONSE);
  Status       = IpmiSubmitCommand (
                   IPMI_NETFN_APP,
                   IPMI_APP_GET_SYSTEM_GUID,
                   (VOID *)NULL,
                   RequestSize,
                   (VOID *)&GetSystemUuidResponse,
                   &ResponseSize
                   );
  if (!EFI_ERROR (Status) && (GetSystemUuidResponse.CompletionCode == IPMI_COMP_CODE_NORMAL)) {
    CopyMem (
      (VOID *)SystemGuid,
      (VOID *)&GetSystemUuidResponse.SystemUuid,
      sizeof (EFI_GUID)
      );
  }

  return Status;
}

/**
  This function gets the channel information.

  @param[in]   GetChannelInfoRequest          The get channel information request.
  @param[out]  GetChannelInfoResponse         The get channel information response.
  @param[out]  GetChannelInfoResponseSize     When input, the expected size of response.
                                              When output, the exact size of the returned
                                              response.

  @retval EFI_SUCCESS            Get channel information successfully.
  @retval EFI_INVALID_PARAMETER  One of the given input parameters is invalid.
  @retval Others                 See the return values of IpmiSubmitCommand () function.

**/
EFI_STATUS
EFIAPI
IpmiGetChannelInfo (
  IN  IPMI_GET_CHANNEL_INFO_REQUEST   *GetChannelInfoRequest,
  OUT IPMI_GET_CHANNEL_INFO_RESPONSE  *GetChannelInfoResponse,
  OUT UINT32                          *GetChannelInfoResponseSize
  )
{
  EFI_STATUS  Status;

  if ((GetChannelInfoRequest == NULL) ||
      (GetChannelInfoResponse == NULL) ||
      (GetChannelInfoResponseSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *GetChannelInfoResponseSize = sizeof (IPMI_GET_CHANNEL_INFO_RESPONSE);
  Status                      = IpmiSubmitCommand (
                                  IPMI_NETFN_APP,
                                  IPMI_APP_GET_CHANNEL_INFO,
                                  (UINT8 *)GetChannelInfoRequest,
                                  sizeof (IPMI_GET_CHANNEL_INFO_REQUEST),
                                  (UINT8 *)GetChannelInfoResponse,
                                  GetChannelInfoResponseSize
                                  );
  return Status;
}

/**
  This function gets system interface capability

  @param[in]  InterfaceCapabilityRequest    Get system interface capability request.
  @param[out] InterfaceCapabilityResponse   The response of system interface capability.
                                            That is caller's responsibility to allocate
                                            memory for the response data.

  @retval EFI_SUCCESS            Command is sent successfully.
  @retval Other                  Failure.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemInterfaceCapability (
  IN  IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST   *InterfaceCapabilityRequest,
  OUT IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_RESPONSE  *InterfaceCapabilityResponse
  )
{
  UINT8       InterfaceType;
  UINT32      ResponseSize;
  UINT32      ActualResponseSize;
  UINT8       *ResponsePtr;
  EFI_STATUS  Status;

  if (InterfaceCapabilityRequest == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InterfaceType = InterfaceCapabilityRequest->Bits.InterfaceType;
  if ((InterfaceType != IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_SSIF) &&
      (InterfaceType != IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_KCS) &&
      (InterfaceType != IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_SMIC))
  {
    DEBUG ((DEBUG_ERROR, "%a: Unsupported given system interface type = 0x%x.\n", __func__, InterfaceType));
    return EFI_INVALID_PARAMETER;
  }

  if (InterfaceType == IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_SSIF) {
    ResponseSize = sizeof (IPMI_GET_SYSTEM_INTERFACE_SSIF_CAPABILITIES_RESPONSE);
    ResponsePtr  = (UINT8 *)InterfaceCapabilityResponse->InterfaceSsifCapability;
  } else {
    ResponseSize = sizeof (IPMI_GET_SYSTEM_INTERFACE_KCS_SMIC_CAPABILITIES_RESPONSE);
    ResponsePtr  = (UINT8 *)InterfaceCapabilityResponse->InterfaceKcsSmicCapability;
  }

  ActualResponseSize = ResponseSize;
  Status             = IpmiSubmitCommand (
                         IPMI_NETFN_APP,
                         IPMI_APP_GET_SYSTEM_INTERFACE_CAPABILITIES,
                         (UINT8 *)InterfaceCapabilityRequest,
                         sizeof (IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST),
                         ResponsePtr,
                         &ActualResponseSize
                         );
  if (ActualResponseSize != ResponseSize) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: The expected response size 0x%x is not equal to the returned size 0x%x.\n",
      __func__,
      ResponseSize,
      ActualResponseSize
      ));
  }

  return Status;
}
