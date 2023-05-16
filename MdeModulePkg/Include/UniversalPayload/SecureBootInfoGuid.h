/** @file
  This file defines the hob structure for the Secure boot information.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SECUREBOOT_INFO_GUID_H_
#define SECUREBOOT_INFO_GUID_H_

#include <UniversalPayload/UniversalPayload.h>

/**
  Secure Boot info Hob GUID
**/
extern EFI_GUID  gUniversalPayloadSecureBootInfoGuid;

#define PAYLOAD_SECUREBOOT_INFO_HOB_REVISION  0x1

#define NO_TPM       0x0
#define TPM_TYPE_12  0x1
#define TPM_TYPE_20  0x2

#pragma pack(1)
typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  UINT8                               VerifiedBootEnabled;
  UINT8                               MeasuredBootEnabled;
  UINT8                               FirmwareDebuggerInitialized;
  UINT8                               TpmType;
  UINT8                               Reserved[3];
  UINT32                              TpmPcrActivePcrBanks;
} UNIVERSAL_SECURE_BOOT_INFO;
#pragma pack()

#endif // SECUREBOOT_INFO_GUID_H_
