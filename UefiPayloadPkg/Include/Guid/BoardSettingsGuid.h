/** @file
  This file defines the hob structure for board settings

  Copyright (c) 2020, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BOARD_SETTINGS_GUID_H__
#define __BOARD_SETTINGS_GUID_H__

///
/// Board information GUID
///
extern EFI_GUID gEfiBoardSettingsVariableGuid;

#pragma pack(1)

typedef struct {
  UINT32 Signature;
  UINT8 SecureBoot;
  UINT8 PrimaryVideo;
  UINT8 DeepSx;
  UINT8 WakeOnUSB;
  UINT8 USBPowerinS5;
  UINT8 PowerStateAfterG3;
  UINT8 BlueRearPort;
  UINT8 InternalAudioConnection;
  UINT8 PxeBootCapability;
} BOARD_SETTINGS;

#pragma pack()

#define BOARD_SETTINGS_NAME L"BoardSettings"

#endif // __BOARD_SETTINGS_GUID_H__
