/** @file
  IPMI Command - NetFnApp NULL instance library.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function gets the IPMI Device ID.

  @param[out] DeviceId  Get device ID response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetDeviceId (
  OUT IPMI_GET_DEVICE_ID_RESPONSE  *DeviceId
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets the self-test result.

  @param[out] SelfTestResult  Self test command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSelfTestResult (
  OUT IPMI_SELF_TEST_RESULT_RESPONSE  *SelfTestResult
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function resets watchdog timer.

  @param[out] CompletionCode  The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiResetWatchdogTimer (
  OUT UINT8  *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets watchdog timer.

  @param[in] SetWatchdogTimer  Set watchdog timer request.
  @param[out] CompletionCode   The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetWatchdogTimer (
  IN  IPMI_SET_WATCHDOG_TIMER_REQUEST  *SetWatchdogTimer,
  OUT UINT8                            *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets watchdog timer.

  @param[out] GetWatchdogTimer  Get watchdog timer response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetWatchdogTimer (
  OUT IPMI_GET_WATCHDOG_TIMER_RESPONSE  *GetWatchdogTimer
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets BMC global enables.

  @param[in]  SetBmcGlobalEnables    Set BMC global enables command request.
  @param[out] CompletionCode         The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetBmcGlobalEnables (
  IN  IPMI_SET_BMC_GLOBAL_ENABLES_REQUEST  *SetBmcGlobalEnables,
  OUT UINT8                                *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets BMC global enables.

  @param[out]  GetBmcGlobalEnables  Get BMC global enables command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetBmcGlobalEnables (
  OUT IPMI_GET_BMC_GLOBAL_ENABLES_RESPONSE  *GetBmcGlobalEnables
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function clears message flag.

  @param[in]    ClearMessageFlagsRequest  Clear message flags command Request.
  @param[out]   CompletionCode            The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiClearMessageFlags (
  IN  IPMI_CLEAR_MESSAGE_FLAGS_REQUEST  *ClearMessageFlagsRequest,
  OUT UINT8                             *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets message flags.

  @param[out]    GetMessageFlagsResponse  Get message flags response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetMessageFlags (
  OUT IPMI_GET_MESSAGE_FLAGS_RESPONSE  *GetMessageFlagsResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets message.

  @param[out]     GetMessageResponse      Get message command response.
  @param[in,out]  GetMessageResponseSize  The size of get message response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetMessage (
  OUT IPMI_GET_MESSAGE_RESPONSE  *GetMessageResponse,
  IN OUT UINT32                  *GetMessageResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sends message.

  @param[in]     SendMessageRequest        The send message command request.
  @param[in]     SendMessageRequestSize    The size of the send message command request.
  @param[out]    SendMessageResponse       The send message command response.
  @param[in,out] SendMessageResponseSize   The size of the send message command response.
                                           When input, the expected size of response.
                                           When output, the actual size of response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

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
  return RETURN_UNSUPPORTED;
}

/**
  This function gets the system UUID.

  @param[out] SystemGuid   The pointer to retrieve system UUID.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.
**/
EFI_STATUS
EFIAPI
IpmiGetSystemUuid (
  OUT EFI_GUID  *SystemGuid
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets the channel information.

  @param[in] GetChannelInfoRequest           The get channel information request.
  @param[out] GetChannelInfoResponse         The get channel information response.
  @param[out] GetChannelInfoResponseSize     When input, the expected size of response.
                                             When output, the exact size of the returned
                                             response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetChannelInfo (
  IN  IPMI_GET_CHANNEL_INFO_REQUEST   *GetChannelInfoRequest,
  OUT IPMI_GET_CHANNEL_INFO_RESPONSE  *GetChannelInfoResponse,
  OUT UINT32                          *GetChannelInfoResponseSize
  )
{
  return RETURN_UNSUPPORTED;
}
