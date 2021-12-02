/** @file
  Define NVData structures used by the HTTP Boot configuration component.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HTTP_BOOT_NVDATA_STRUC_H_
#define _HTTP_BOOT_NVDATA_STRUC_H_

#include <Guid/HttpBootConfigHii.h>

#define HTTP_BOOT_IP_VERSION_4  0
#define HTTP_BOOT_IP_VERSION_6  1

//
// Macros used for an IPv4 or an IPv6 address.
//
#define URI_STR_MIN_SIZE  0
#define URI_STR_MAX_SIZE  255

#define DESCRIPTION_STR_MIN_SIZE  6
#define DESCRIPTION_STR_MAX_SIZE  75

#define CONFIGURATION_VARSTORE_ID  0x1234

#define FORMID_MAIN_FORM  1

#define KEY_INITIATOR_URI  0x101

#define HTTP_BOOT_DEFAULT_DESCRIPTION_STR  L"UEFI HTTP"

#pragma pack(1)
typedef struct _HTTP_BOOT_CONFIG_IFR_NVDATA {
  UINT8     IpVersion;
  UINT8     Padding;
  CHAR16    Description[DESCRIPTION_STR_MAX_SIZE];
  CHAR16    Uri[URI_STR_MAX_SIZE];
} HTTP_BOOT_CONFIG_IFR_NVDATA;
#pragma pack()

#endif
