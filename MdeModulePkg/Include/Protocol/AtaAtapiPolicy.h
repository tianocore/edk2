/** @file
  ATA ATAPI Policy protocol is produced by platform and consumed by AtaAtapiPassThruDxe
  driver.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __ATA_ATAPI_POLICY_H__
#define __ATA_ATAPI_POLICY_H__

#define EDKII_ATA_ATAPI_POLICY_PROTOCOL_GUID \
  { \
    0xe59cd769, 0x5083, 0x4f26,{ 0x90, 0x94, 0x6c, 0x91, 0x9f, 0x91, 0x6c, 0x4e } \
  }

typedef struct {
  ///
  /// Protocol version.
  ///
  UINT32  Version;

  ///
  /// 0: Disable Power-up in Standby;
  /// 1: Enable Power-up in Standby;
  /// others: Since PUIS setting is non-volatile, platform can use other value than 0/1 to keep hardware PUIS setting.
  ///
  UINT8   PuisEnable;

  ///
  /// 0: Disable Device Sleep;
  /// 1: Enable Device Sleep;
  /// others: Ignored.
  ///
  UINT8   DeviceSleepEnable;

  ///
  /// 0: Disable Aggressive Device Sleep;
  /// 1: Enable Aggressive Device Sleep;
  /// others: Ignored.
  ///
  UINT8   AggressiveDeviceSleepEnable;

  UINT8   Reserved;
} EDKII_ATA_ATAPI_POLICY_PROTOCOL;

#define EDKII_ATA_ATAPI_POLICY_VERSION 0x00010000


extern EFI_GUID gEdkiiAtaAtapiPolicyProtocolGuid;

#endif

