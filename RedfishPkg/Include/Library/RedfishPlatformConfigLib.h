/** @file
  Definitions of RedfishPlatformConfigLib

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_PLATFORM_CONFIG_LIB_H_
#define REDFISH_PLATFORM_CONFIG_LIB_H_

#include <Protocol/EdkIIRedfishPlatformConfig.h>

/**
  Get Redfish value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  OUT    EDKII_REDFISH_VALUE  *Value
  );

/**
  Set Redfish value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   Value               The value to set.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigSetValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  IN     EDKII_REDFISH_VALUE  Value
  );

/**
  Get the list of Configure Language from platform configuration by the given Schema and Pattern.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   Pattern             The target Configure Language pattern.
  @param[out]  ConfigureLangList   The list of Configure Language.
  @param[out]  Count               The number of Configure Language in ConfigureLangList.

  @retval EFI_SUCCESS              ConfigureLangList is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetConfigureLang (
  IN     CHAR8       *Schema,
  IN     CHAR8       *Version,
  IN     EFI_STRING  Pattern,
  OUT    EFI_STRING  **ConfigureLangList,
  OUT    UINTN       *Count
  );

/**
  Get the list of supported Redfish schema from platform configuration.

  @param[out]  SupportedSchema     The supported schema list which is separated by ';'.
                                   For example: "x-UEFI-redfish-Memory.v1_7_1;x-UEFI-redfish-Boot.v1_0_1"
                                   The SupportedSchema is allocated by the callee. It's caller's
                                   responsibility to free this buffer using FreePool().

  @retval EFI_SUCCESS              Schema is returned successfully.
  @retval EFI_NOT_READY            Redfish Platform Config protocol is not ready.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigGetSupportedSchema (
  OUT    CHAR8  **SupportedSchema
  );

/**
  Get Redfish attribute value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  AttributeValue      The attribute value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetAttribute (
  IN     CHAR8                    *Schema,
  IN     CHAR8                    *Version,
  IN     EFI_STRING               ConfigureLang,
  OUT    EDKII_REDFISH_ATTRIBUTE  *AttributeValue
  );

/**
  Get Redfish default value with the given Schema and Configure Language.

  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   DefaultClass        The UEFI defined default class.
                                   Please refer to UEFI spec. 33.2.5.8 "defaults" for details.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishPlatformConfigGetDefaultValue (
  IN     CHAR8                *Schema,
  IN     CHAR8                *Version,
  IN     EFI_STRING           ConfigureLang,
  IN     UINT16               DefaultClass,
  OUT    EDKII_REDFISH_VALUE  *Value
  );

#endif
