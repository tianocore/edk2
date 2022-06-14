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
extern EFI_GUID gEfiBoardBootOverrideVariableGuid;

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
  UINT8 PinkRearPort;
  UINT8 VtxDisabled;
  UINT8 MenuDisabled;
} BOARD_SETTINGS;

#define PRIMARY_VIDEO_ASPEED 0
#define PRIMARY_VIDEO_INTEL 1
#define PRIMARY_VIDEO_SLOT1 2
#define PRIMARY_VIDEO_SLOT2 3
#define PRIMARY_VIDEO_SLOT3 4
#define PRIMARY_VIDEO_SLOT4 5
#define PRIMARY_VIDEO_SLOT5 6
#define PRIMARY_VIDEO_SLOT6 7

typedef struct {
  UINT16 StructSize;
  UINT32 Checksum;
  UINT8  Flags;
  UINT8  BootOptionOverride;
  UINT8  Port;
} BOARD_BOOT_OVERRIDE;

enum BoardBootOverride {
  BootOverrideNone = 0,
  BootOverridePXE = 1,
  BootOverrideSATA = 2,
  BootOverrideNVME = 3,
  BootOverrideCD = 4,
  BootOverrideSetupMenu = 5,
  BootOverrideUSB = 6,
  BootOverrideMax
};

enum iPXEBootOverride {
  IPV4IPV6 = 0,
  IPV4 = 1,
  IPV6 = 2,
};

typedef struct {
  UINT8 Type;
  UINT8 Port;
} BOOT_OVERRIDE;

#pragma pack()

#define BOARD_SETTINGS_NAME L"BoardSettings"
#define BOARD_BOOT_OVERRIDE_NAME L"BoardBootOverride"

#endif // __BOARD_SETTINGS_GUID_H__
