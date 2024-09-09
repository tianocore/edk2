/** @file
  This file defines the EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL interface.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_PLATFORM_CONFIG_H_
#define EDKII_REDFISH_PLATFORM_CONFIG_H_

typedef struct _EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL;

//
// Redfish Platform Config Protocol interface version.
//
#define REDFISH_PLATFORM_CONFIG_VERSION  0x00010000

///
/// Definition of EDKII_REDFISH_TYPE_VALUE
///
typedef union {
  INT64      Integer;
  BOOLEAN    Boolean;
  CHAR8      *Buffer;
  CHAR8      **StringArray;
  INT64      *IntegerArray;
  BOOLEAN    *BooleanArray;
} EDKII_REDFISH_TYPE_VALUE;

///
/// Definition of EDKII_REDFISH_VALUE_TYPES
///
typedef enum {
  RedfishValueTypeUnknown = 0,
  RedfishValueTypeInteger,
  RedfishValueTypeBoolean,
  RedfishValueTypeString,
  RedfishValueTypeStringArray,
  RedfishValueTypeIntegerArray,
  RedfishValueTypeBooleanArray,
  RedfishValueTypeMax
} EDKII_REDFISH_VALUE_TYPES;

///
/// Definition of EDKII_REDFISH_ATTRIBUTE_TYPES
///
typedef enum {
  RedfishAttributeTypeUnknown = 0,
  RedfishAttributeTypeEnumeration,
  RedfishAttributeTypeString,
  RedfishAttributeTypeInteger,
  RedfishAttributeTypeBoolean,
  RedfishAttributeTypePassword
} EDKII_REDFISH_ATTRIBUTE_TYPES;

///
/// Definition of EDKII_REDFISH_VALUE
///
typedef struct {
  EDKII_REDFISH_VALUE_TYPES    Type;
  EDKII_REDFISH_TYPE_VALUE     Value;
  UINTN                        ArrayCount;
} EDKII_REDFISH_VALUE;

///
/// Definition of EDKII_REDFISH_ATTRIBUTE_VALUE
///
typedef struct {
  CHAR8    *ValueName;
  CHAR8    *ValueDisplayName;
} EDKII_REDFISH_ATTRIBUTE_VALUE;

///
/// Definition of EDKII_REDFISH_POSSIBLE_VALUES
///
typedef struct {
  UINTN                            ValueCount;
  EDKII_REDFISH_ATTRIBUTE_VALUE    *ValueArray;
} EDKII_REDFISH_POSSIBLE_VALUES;

///
/// Definition of EDKII_REDFISH_ATTRIBUTE
///
typedef struct {
  CHAR8                            *AttributeName;
  CHAR8                            *DisplayName;
  CHAR8                            *HelpText;
  CHAR8                            *MenuPath;
  EDKII_REDFISH_ATTRIBUTE_TYPES    Type;
  BOOLEAN                          ResetRequired;
  BOOLEAN                          ReadOnly;
  BOOLEAN                          GrayedOut;
  BOOLEAN                          Suppress;
  UINT64                           NumMaximum;
  UINT64                           NumMinimum;
  UINT64                           NumStep;
  UINT8                            StrMaxSize;
  UINT8                            StrMinSize;
  EDKII_REDFISH_POSSIBLE_VALUES    Values;
} EDKII_REDFISH_ATTRIBUTE;

/**
  Get Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_GET_VALUE)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  OUT    EDKII_REDFISH_VALUE                    *Value
  );

//
// Default class standard
//
#define EDKII_REDFISH_DEFAULT_CLASS_STANDARD  EFI_HII_DEFAULT_CLASS_STANDARD

/**
  Get Redfish default value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   DefaultClass        The UEFI defined default class.
                                   Please refer to UEFI spec. 33.2.5.8 "defaults" for details.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_GET_DEFAULT_VALUE)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  IN     UINT16                                 DefaultClass,
  OUT    EDKII_REDFISH_VALUE                    *Value
  );

/**
  Set Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   Value               The value to set.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_SET_VALUE)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  IN     EDKII_REDFISH_VALUE                    Value
  );

/**
  Get Redfish attribute value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  AttributeValue      The attribute value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_GET_ATTRIBUTE)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  OUT    EDKII_REDFISH_ATTRIBUTE                *AttributeValue
  );

/**
  Get the list of Configure Language from platform configuration by the given Schema and RegexPattern.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   RegexPattern        The target Configure Language pattern. This is used for regular expression matching.
  @param[out]  ConfigureLangList   The list of Configure Language.
  @param[out]  Count               The number of Configure Language in ConfigureLangList.

  @retval EFI_SUCCESS              ConfigureLangList is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_GET_CONFIG_LANG)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             RegexPattern,
  OUT    EFI_STRING                             **ConfigureLangList,
  OUT    UINTN                                  *Count
  );

/**
  Get the list of supported Redfish schema from platform configuration.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[out]  SupportedSchema     The supported schema list which is separated by ';'.
                                   For example: "x-UEFI-redfish-Memory.v1_7_1;x-UEFI-redfish-Boot.v1_0_1"
                                   The SupportedSchema is allocated by the callee. It's caller's
                                   responsibility to free this buffer using FreePool().

  @retval EFI_SUCCESS              Schema is returned successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_PLATFORM_CONFIG_GET_SUPPORTED_SCHEMA)(
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL    *This,
  OUT    CHAR8                                     **SupportedSchema
  );

struct _EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL {
  UINT64                                                Revision;
  EDKII_REDFISH_PLATFORM_CONFIG_GET_VALUE               GetValue;
  EDKII_REDFISH_PLATFORM_CONFIG_SET_VALUE               SetValue;
  EDKII_REDFISH_PLATFORM_CONFIG_GET_DEFAULT_VALUE       GetDefaultValue;
  EDKII_REDFISH_PLATFORM_CONFIG_GET_ATTRIBUTE           GetAttribute;
  EDKII_REDFISH_PLATFORM_CONFIG_GET_CONFIG_LANG         GetConfigureLang;
  EDKII_REDFISH_PLATFORM_CONFIG_GET_SUPPORTED_SCHEMA    GetSupportedSchema;
};

extern EFI_GUID  gEdkIIRedfishPlatformConfigProtocolGuid;

#endif
