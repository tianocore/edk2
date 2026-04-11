/** @file
  IPMI Command - NetFnChassis NULL instance library.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

/**
  This function gets chassis capability.

  @param[out] GetChassisCapabilitiesResponse  Gets chassis capability command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisCapabilities (
  OUT IPMI_GET_CHASSIS_CAPABILITIES_RESPONSE  *GetChassisCapabilitiesResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets chassis status.

  @param[out] GetChassisStatusResponse  The get chassis status command response.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetChassisStatus (
  OUT IPMI_GET_CHASSIS_STATUS_RESPONSE  *GetChassisStatusResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sends chassis control request.

  @param[in]  ChassisControlRequest  The chassis control request.
  @param[out] CompletionCode         The command completion code.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiChassisControl (
  IN IPMI_CHASSIS_CONTROL_REQUEST  *ChassisControlRequest,
  OUT UINT8                        *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets power restore policy.

  @param[in]  ChassisControlRequest    The set power restore policy control
                                       command request.
  @param[out] ChassisControlResponse   The response of power restore policy.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetPowerRestorePolicy (
  IN  IPMI_SET_POWER_RESTORE_POLICY_REQUEST   *ChassisControlRequest,
  OUT IPMI_SET_POWER_RESTORE_POLICY_RESPONSE  *ChassisControlResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function sets system boot option.

  @param[in]  BootOptionsRequest    Set system boot option request.
  @param[out] BootOptionsResponse   The response of set system boot
                                    option request.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiSetSystemBootOptions (
  IN  IPMI_SET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_SET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  This function gets system boot option.

  @param[in]  BootOptionsRequest    Get system boot option request.
  @param[out] BootOptionsResponse   The response of get system boot
                                    option request.

  @retval EFI_UNSUPPORTED  Unsupported in the NULL lib.

**/
EFI_STATUS
EFIAPI
IpmiGetSystemBootOptions (
  IN  IPMI_GET_BOOT_OPTIONS_REQUEST   *BootOptionsRequest,
  OUT IPMI_GET_BOOT_OPTIONS_RESPONSE  *BootOptionsResponse
  )
{
  return RETURN_UNSUPPORTED;
}
