/** @file
  HddPassword HII data structure used by the driver.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HDD_PASSWORD_HII_DATASTRUC_H_
#define _HDD_PASSWORD_HII_DATASTRUC_H_

#include <Guid/HiiPlatformSetupFormset.h>

#define HDD_PASSWORD_CONFIG_GUID \
  { \
    0x737cded7, 0x448b, 0x4801, { 0xb5, 0x7d, 0xb1, 0x94, 0x83, 0xec, 0x60, 0x6f } \
  }

#define FORMID_HDD_MAIN_FORM          1
#define FORMID_HDD_DEVICE_FORM        2

#define HDD_DEVICE_ENTRY_LABEL        0x1234
#define HDD_DEVICE_LABEL_END          0xffff

#define KEY_HDD_DEVICE_ENTRY_BASE     0x1000

#define KEY_HDD_USER_PASSWORD         0x101
#define KEY_HDD_MASTER_PASSWORD       0x102

#pragma pack(1)

typedef struct {
  UINT8     Supported:1;
  UINT8     Enabled:1;
  UINT8     Locked:1;
  UINT8     Frozen:1;
  UINT8     UserPasswordStatus:1;
  UINT8     MasterPasswordStatus:1;
  UINT8     Reserved:2;
} HDD_PASSWORD_SECURITY_STATUS;

typedef struct {
  UINT8     UserPassword:1;
  UINT8     MasterPassword:1;
  UINT8     Reserved:6;
} HDD_PASSWORD_REQUEST;

typedef struct _HDD_PASSWORD_CONFIG {
  HDD_PASSWORD_SECURITY_STATUS  SecurityStatus;
  HDD_PASSWORD_REQUEST          Request;
} HDD_PASSWORD_CONFIG;

#pragma pack()

#endif
