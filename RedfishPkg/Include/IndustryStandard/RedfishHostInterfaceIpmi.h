/** @file
  Redfish Host Interface IPMI command

  Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef REDFISH_HOST_INTERFACE_IPMI_H_
#define REDFISH_HOST_INTERFACE_IPMI_H_

#include <Uefi.h>
#include <IndustryStandard/IpmiNetFnGroupExtension.h>

#define REDFISH_IPMI_GROUP_EXTENSION                          0x52
#define REDFISH_IPMI_GET_BOOTSTRAP_CREDENTIALS_CMD            0x02
#define REDFISH_IPMI_BOOTSTRAP_CREDENTIAL_ENABLE              0xA5
#define REDFISH_IPMI_BOOTSTRAP_CREDENTIAL_DISABLE             0x00
#define REDFISH_IPMI_COMP_CODE_BOOTSTRAP_CREDENTIAL_DISABLED  0x80

///
/// Per Redfish Host Interface Specification 1.3, The maximum length of
/// username and password is 16 characters long.
//
#define USERNAME_MAX_LENGTH  16
#define PASSWORD_MAX_LENGTH  16
#define USERNAME_MAX_SIZE    (USERNAME_MAX_LENGTH + 1)  // NULL terminator
#define PASSWORD_MAX_SIZE    (PASSWORD_MAX_LENGTH + 1)  // NULL terminator

#pragma pack(1)

///
/// The definition of IPMI command to get bootstrap account credentials
///
typedef struct {
  UINT8    GroupExtensionId;
  UINT8    DisableBootstrapControl;
} IPMI_BOOTSTRAP_CREDENTIALS_COMMAND_DATA;

///
/// The response data of getting bootstrap credential
///
typedef struct {
  UINT8    CompletionCode;
  UINT8    GroupExtensionId;
  CHAR8    Username[USERNAME_MAX_LENGTH];
  CHAR8    Password[PASSWORD_MAX_LENGTH];
} IPMI_BOOTSTRAP_CREDENTIALS_RESULT_RESPONSE;

#pragma pack()

#endif
